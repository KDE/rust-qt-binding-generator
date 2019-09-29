extern crate cc;

use super::{generate_bindings, read_bindings_file, Config};
use regex::Regex;
use serde_xml_rs::deserialize;
use std::env::var;
use std::io::{self, Write};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Debug, Deserialize)]
struct RCC {
    qresource: Vec<QResource>,
}

#[derive(Debug, Deserialize)]
struct QResource {
    prefix: String,
    file: Vec<PathBuf>,
}

/// Parse the qrc file, panic if it fails.
fn read_qrc(qrc: &Path) -> RCC {
    let bytes =
        ::std::fs::read(qrc).unwrap_or_else(|e| panic!("Could not read {}: {}", qrc.display(), e));
    let mut rcc: RCC = deserialize(&bytes[..])
        .unwrap_or_else(|e| panic!("could not parse {}: {}", qrc.display(), e));
    for qresource in &mut rcc.qresource {
        for file in &mut qresource.file {
            let mut p = qrc.parent().unwrap().to_path_buf();
            p.push(&file);
            *file = p;
        }
    }
    rcc
}

/// Get the list of files that are listed in the qrc file.
fn qrc_to_input_list<'a>(qrc: &'a Path, rcc: &'a RCC) -> Vec<&'a Path> {
    let mut list = Vec::new();
    list.push(qrc);
    for qresource in &rcc.qresource {
        for file in &qresource.file {
            list.push(file);
        }
    }
    list
}

/// Run a commmand and return the standard output if the command ran ok.
/// Otherwise print an error and exit this program.
fn run(cmd: &str, command: &mut Command) -> Vec<u8> {
    eprintln!("running: {:?}", command);
    match command.output() {
        Err(e) => eprintln!(
            "Could not run {}. Make sure {} is in your path: {}",
            cmd, cmd, e
        ),
        Ok(output) => {
            io::stderr()
                .write_all(&output.stderr)
                .expect("Could not write to stderr.");
            if output.status.success() {
                return output.stdout;
            }
            match output.status.code() {
                None => eprintln!("Process {} terminated by signal.", cmd),
                Some(code) => eprintln!("{} exited with status code: {}", cmd, code),
            }
        }
    }
    ::std::process::exit(-1);
}

/// Query a Qt environment variable via qmake.
fn qmake_query(var: &str) -> String {
    let v = String::from_utf8(run("qmake", Command::new("qmake").args(&["-query", var])))
        .expect("qmake output was not valid UTF-8");
    v.trim().to_string()
}

fn qt_include_path() -> PathBuf {
    let qt_include_path = PathBuf::from(qmake_query("QT_INSTALL_HEADERS"));
    if qt_include_path.exists() {
        return qt_include_path;
    }
    // NixOS workaround: get value from CMAKE env variable
    let path = var("CMAKE_INCLUDE_PATH").expect("CMAKE_INCLUDE_PATH is not set");
    let paths = path.split(':');
    for path in paths {
        if path.contains("qtbase") {
            return path.into();
        }
    }
    panic!("Could not find QT_INSTALL_HEADERS")
}

struct Version {
    major: u8,
    minor: u8,
    patch: u8,
}

/// Return the qt version number as an integer.
///
/// # Panics
///
/// Panic if the value could not be parsed.
fn parse_qt_version(qt_version: &str) -> Version {
    let re = Regex::new(r"(\d)\.(\d{1,2})(\.(\d{1,2}))").unwrap();
    match re.captures(&qt_version) {
        None => panic!("Cannot parse Qt version number {}", qt_version),
        Some(cap) => Version {
            major: cap[1].parse::<u8>().unwrap(),
            minor: cap[2].parse::<u8>().unwrap(),
            patch: cap
                .get(4)
                .map(|m| m.as_str())
                .unwrap_or("0")
                .parse::<u8>()
                .unwrap(),
        },
    }
}

/// Check for a minimal Qt version.
///
/// # Example
///
/// ```
/// # use std::env;
/// # use rust_qt_binding_generator::build::require_qt_version;
/// require_qt_version(5, 1, 0);
/// ```
///
/// # Panics
///
/// Panics if the installed version is smaller than the required version or if
/// no version of Qt could be determined.
pub fn require_qt_version(major: u8, minor: u8, patch: u8) {
    let qt_version = qmake_query("QT_VERSION");
    let version = parse_qt_version(&qt_version);
    if version.major < major
        || (version.major == major
            && (version.minor < minor || (version.minor == minor && version.patch < patch)))
    {
        panic!(
            "Please use a version of Qt >= {}.{}.{}, not {}",
            major, minor, patch, qt_version
        );
    }
}

#[derive(Debug)]
pub enum QtModule {
    Core,
    Gui,
    Multimedia,
    MultimediaWidgets,
    Network,
    Qml,
    Quick,
    QuickTest,
    Sql,
    Test,
    Widgets,

    Concurrent,
    DBus,
    Help,
    Location,
    OpenGL,
    Positioning,
    PrintSupport,
    Svg,
    WebChannel,
    WebEngine,
    WaylandCompositor,
    X11Extras,
    Xml,
    XmlPatterns,

    Charts,
}

/// A builder for binding generation and compilation of a Qt application.
///
/// Pass options into this `Build` and then run `build` to generate bindings
/// and compile the Qt C++ code and resources into a static library.
/// This struct is meant to be used in a `build.rs` script.
pub struct Build {
    qt_library_path: PathBuf,
    qt_include_path: PathBuf,
    out_dir: PathBuf,
    build: cc::Build,
    bindings: Vec<PathBuf>,
    qrc: Vec<PathBuf>,
    ui: Vec<PathBuf>,
    h: Vec<PathBuf>,
    cpp: Vec<PathBuf>,
    modules: Vec<QtModule>,
    link_libs: Vec<PathBuf>,
}

impl Build {
    /// Create a new `Build` struct.
    ///
    /// Initialize the struct with the build output directory. That directory
    /// is available via the environment variable `OUT_DIR`.
    ///
    /// # Example
    ///
    /// ```
    /// use std::env;
    /// use rust_qt_binding_generator::build::Build;
    /// if let Ok(out_dir) = env::var("OUT_DIR") {
    ///     Build::new(&out_dir)
    ///         .bindings("bindings.json")
    ///         .qrc("qml.qrc")
    ///         .ui("main.ui")
    ///         .cpp("src/main.cpp")
    ///         .compile("my_app");
    /// }
    /// ```
    pub fn new<P: AsRef<Path>>(out_dir: P) -> Build {
        let qt_include_path = qt_include_path();
        let mut build = cc::Build::new();
        build
            .cpp(true)
            .include(out_dir.as_ref())
            .include(&qt_include_path);
        Build {
            qt_library_path: qmake_query("QT_INSTALL_LIBS").into(),
            qt_include_path,
            out_dir: out_dir.as_ref().to_path_buf(),
            build,
            bindings: Vec::new(),
            qrc: Vec::new(),
            ui: Vec::new(),
            h: Vec::new(),
            cpp: Vec::new(),
            modules: vec![QtModule::Core],
            link_libs: Vec::new(),
        }
    }
    /// Add a bindings file to be processed.
    pub fn bindings<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.bindings.push(path.as_ref().to_path_buf());
        self
    }
    /// Add a qrc file to be processed.
    ///
    /// qrc files are Qt Resource files. Files listed in a qrc file are
    /// compiled into a the binary. Here is an example qrc file that adds a qml
    /// file.
    /// ```xml
    /// <RCC>
    ///    <qresource prefix="/">
    ///      <file>main.qml</file>
    ///    </qresource>
    /// </RCC>
    /// ```
    pub fn qrc<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.qrc.push(path.as_ref().to_path_buf());
        self
    }
    /// Add a ui file to be processed.
    ///
    /// ui files are generated by Qt Designer and describe how to build the user interface.
    pub fn ui<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.ui.push(path.as_ref().to_path_buf());
        self
    }
    /// Add a C++ header file to be compiled into the program.
    pub fn h<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.h.push(path.as_ref().to_path_buf());
        self
    }
    /// Add a C++ file to be compiled into the program.
    pub fn cpp<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.cpp.push(path.as_ref().to_path_buf());
        self
    }
    /// Add the path to the paths includes for headers.
    pub fn include_path<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.build.include(path);
        self
    }
    /// Link to the specified library.
    pub fn link_lib<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.link_libs.push(path.as_ref().to_path_buf());
        self
    }
    fn print_link_libs(&self) {
        for path in &self.link_libs {
            if let Some(parent) = path.parent() {
                if parent.exists() {
                    println!("cargo:rustc-link-search=native={}", parent.display());
                }
            }
            if let Some(name) = path.file_name().and_then(|a| a.to_str()) {
                // remove 'lib' prefix and '.so...' suffix
                let name = if let Some(pos) = name.find('.') {
                    &name[3..pos]
                } else {
                    &name[3..]
                };
                println!("cargo:rustc-link-lib={}", name);
            }
        }
    }
    /// Add a Qt module to be linked to the executable
    pub fn module(&mut self, module: QtModule) -> &mut Build {
        self.modules.push(module);
        self
    }
    /// Compile the static library.
    ///
    /// # Panics
    ///
    /// Panics if there is any kind of error. Run `cargo build -vv` to
    /// get more output while debugging.
    pub fn compile(&mut self, lib_name: &str) {
        for binding in &self.bindings {
            handle_binding(&self.out_dir, binding, &mut self.h, &mut self.cpp);
        }
        let mut compile_inputs: Vec<&Path> = Vec::new();
        let ui_hs: Vec<_> = self.ui.iter().map(|ui| handle_ui(ui)).collect();
        for ui_h in &ui_hs {
            compile_inputs.push(ui_h);
        }
        for h in &self.h {
            compile_inputs.push(h);
            handle_header(h, &mut self.cpp);
        }
        for qrc in &self.qrc {
            handle_qrc(&self.out_dir, qrc, &mut self.cpp);
        }
        for cpp in &self.cpp {
            compile_inputs.push(cpp);
            self.build.file(cpp);
        }
        // add the Qt module include folders
        for module in &self.modules {
            self.build
                .include(self.qt_include_path.join(&format!("Qt{:?}", module)));
        }
        let lib = self.out_dir.join(&format!("lib{}.a", lib_name));
        if should_run(&compile_inputs, &[&lib]) {
            self.build.compile(lib_name);
        } else {
            // normally cc::Build outputs this information
            println!("cargo:rustc-link-lib=static={}", lib_name);
            println!("cargo:rustc-link-search=native={}", self.out_dir.display());
            print_cpp_link_stdlib();
        }
        println!("cargo:rustc-link-search={}", self.qt_library_path.display());
        for module in &self.modules {
            println!("cargo:rustc-link-lib=Qt5{:?}", module);
        }
        self.print_link_libs();
    }
}

// Print the c++ library that cargo should link against
// This is used when calling 'cargo build' when no actual recompile is needed.
fn print_cpp_link_stdlib() {
    let target = ::std::env::var("TARGET").unwrap();
    let stdlib = if target.contains("msvc") {
        None
    } else if target.contains("apple") || target.contains("freebsd") || target.contains("openbsd") {
        Some("c++")
    } else {
        Some("stdc++")
    };
    if let Some(stdlib) = stdlib {
        println!("cargo:rustc-link-lib={}", stdlib);
    }
}

/// Return true if all outputs and exist are older than the given input time.
fn are_outputs_up_to_date(paths: &[&Path], input: SystemTime) -> bool {
    for path in paths {
        if let Ok(mt) = path.metadata().and_then(|m| m.modified()) {
            if mt <= input {
                let duration = input.duration_since(mt).unwrap();
                println!(
                    "{} is outdated by {} seconds.",
                    path.display(),
                    duration.as_secs()
                );
                return false;
            }
        } else {
            println!("{} does not exist.", path.display());
            return false;
        }
    }
    true
}

/// Return the youngest/newest mtime for the paths.
fn get_youngest_mtime(paths: &[&Path]) -> Result<SystemTime, String> {
    let mut max = UNIX_EPOCH;
    for path in paths {
        let mt = path
            .metadata()
            .and_then(|m| m.modified())
            .map_err(|e| format!("Error reading file {}: {}.", path.display(), e))?;
        if mt > max {
            max = mt;
        }
    }
    Ok(max)
}

/// Run moc to generate C++ code from a Qt C++ header
fn moc(header: &Path, output: &Path) {
    run("moc", Command::new("moc").arg("-o").arg(output).arg(header));
}

/// Run rcc to generate C++ code from a Qt resource file
fn rcc(rcfile: &Path, output: &Path) {
    run("rcc", Command::new("rcc").arg("-o").arg(output).arg(rcfile));
}

/// Run uic to generate C++ code from a QT ui file
fn uic(uifile: &Path, output: &Path) {
    run("uic", Command::new("uic").arg("-o").arg(output).arg(uifile));
}

/// return true if a command should run.
/// It returns true if all inputs are present and if any of the inputs is newer
/// than the newest output or if the outputs do not exist yet.
fn should_run(input: &[&Path], output: &[&Path]) -> bool {
    // get the youngest input time
    let input_time = match get_youngest_mtime(input) {
        Ok(time) => time,
        Err(e) => panic!(e),
    };
    !are_outputs_up_to_date(output, input_time)
}

fn get_interface_module_path(config: &Config) -> PathBuf {
    let mut path = config.rust.dir.join("src");
    path.push(&config.rust.interface_module);
    path.set_extension("rs");
    PathBuf::new()
}

fn handle_binding(
    out_dir: &Path,
    bindings_json: &Path,
    h: &mut Vec<PathBuf>,
    cpp: &mut Vec<PathBuf>,
) {
    let mut config = read_bindings_file(&bindings_json)
        .unwrap_or_else(|e| panic!("Could not parse {}: {}", bindings_json.display(), e));
    let bindings_cpp = out_dir.join(&config.cpp_file);
    let mut bindings_h = bindings_cpp.clone();
    bindings_h.set_extension("h");
    config.cpp_file = bindings_cpp.clone();
    config.rust.dir = out_dir.join(&config.rust.dir);
    let interface_rs = get_interface_module_path(&config);
    if should_run(
        &[&bindings_json],
        &[&bindings_h, &bindings_cpp, &interface_rs],
    ) {
        generate_bindings(&config).unwrap();
    }
    h.push(bindings_h);
    cpp.push(bindings_cpp);
}

fn handle_qrc(out_dir: &Path, qrc_path: &Path, cpp: &mut Vec<PathBuf>) {
    let qrc = read_qrc(qrc_path);
    let qml_cpp = out_dir.join(format!(
        "qrc_{}.cpp",
        qrc_path.file_stem().unwrap().to_str().unwrap()
    ));
    let qrc_inputs = qrc_to_input_list(qrc_path, &qrc);
    if should_run(&qrc_inputs, &[&qml_cpp]) {
        rcc(&qrc_path, &qml_cpp);
    }
    cpp.push(qml_cpp);
}

fn handle_ui(ui: &Path) -> PathBuf {
    let ui_h = ui.parent().unwrap().join(format!(
        "ui_{}.h",
        ui.file_stem().unwrap().to_str().unwrap()
    ));

    if should_run(&[ui], &[&ui_h]) {
        uic(&ui, &ui_h);
    }
    ui_h
}

fn handle_header(h: &Path, cpp: &mut Vec<PathBuf>) {
    let moc_file = h.parent().unwrap().join(format!(
        "moc_{}.cpp",
        h.file_stem().unwrap().to_str().unwrap()
    ));
    if should_run(&[&h], &[&moc_file]) {
        moc(&h, &moc_file);
    }
    cpp.push(moc_file);
}
