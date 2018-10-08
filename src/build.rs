extern crate cc;

use serde_xml_rs::deserialize;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::io::{self, Write};
use super::{Config, generate_bindings, read_config_file};
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

/// Parse the qrc file, panic if it fails
fn read_qrc(qrc: &Path) -> RCC {
    let bytes = ::std::fs::read(qrc).expect(&format!("Could not read {}", qrc.display()));
    deserialize(&bytes[..]).expect(&format!("could not parse {}", qrc.display()))
}

/// Get the list of files that are listed in the qrc file
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

/// run a commmand and return the standard output if the command ran ok
/// otherwise print an error and exit this program
fn run(cmd: &str, command: &mut Command) -> Vec<u8> {
    eprintln!("running: {:?}", command);
    match command.output() {
        Err(e) => {
            eprintln!(
                "Could not run {}. Make sure {} is in your path: {}",
                cmd,
                cmd,
                e
            )
        }
        Ok(output) => {
            io::stderr().write(&output.stderr).expect(
                "Could not write to stderr.",
            );
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

/// query a Qt environment variable via qmake
fn qmake_query(var: &str) -> String {
    String::from_utf8(run("qmake", Command::new("qmake").args(&["-query", var])))
        .expect("qmake output was not valid UTF-8")
}

pub struct Build {
    qt_library_path: PathBuf,
    out_dir: PathBuf,
    build: cc::Build,
    bindings: Vec<PathBuf>,
    qrc: Vec<PathBuf>,
    h: Vec<PathBuf>,
    cpp: Vec<PathBuf>,
}

impl Build {
    pub fn new<P: AsRef<Path>>(out_dir: P) -> Build {
        let qt_include_path = qmake_query("QT_INSTALL_HEADERS");
        let mut build = cc::Build::new();
        build.cpp(true).include(out_dir.as_ref()).include(
            qt_include_path
                .trim(),
        );
        Build {
            qt_library_path: qmake_query("QT_INSTALL_LIBS").trim().into(),
            out_dir: out_dir.as_ref().to_path_buf(),
            build,
            bindings: Vec::new(),
            qrc: Vec::new(),
            h: Vec::new(),
            cpp: Vec::new(),
        }
    }
    pub fn bindings<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.bindings.push(path.as_ref().to_path_buf());
        self
    }
    pub fn qrc<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.qrc.push(path.as_ref().to_path_buf());
        self
    }
    pub fn cpp<P: AsRef<Path>>(&mut self, path: P) -> &mut Build {
        self.cpp.push(path.as_ref().to_path_buf());
        self
    }
    pub fn compile(&mut self, lib_name: &str) {
        for binding in &self.bindings {
            handle_binding(&self.out_dir, binding, &mut self.h, &mut self.cpp);
        }
        for h in &self.h {
            handle_header(h, &mut self.cpp);
        }
        for qrc in &self.qrc {
            handle_qrc(&self.out_dir, qrc, &mut self.cpp);
        }
        for cpp in &self.cpp {
            self.build.file(cpp);
        }
        let lib = self.out_dir.join(&format!("lib{}.a", lib_name));
        let inputs = Vec::new();
        if should_run(&inputs, &[&lib]) {
            self.build.compile(lib_name);
        } else {
            // normally cc::Build outputs this information
            println!("cargo:rustc-link-lib=static={}", lib_name);
            println!("cargo:rustc-link-search=native={}", self.out_dir.display());
            println!("cargo:rustc-link-lib=stdc++");
        }
        println!("cargo:rustc-link-search={}", self.qt_library_path.display());
        println!("cargo:rustc-link-lib=Qt5Core");
        println!("cargo:rustc-link-lib=Qt5Network");
        println!("cargo:rustc-link-lib=Qt5Gui");
        println!("cargo:rustc-link-lib=Qt5Qml");
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
        let mt = path.metadata().and_then(|m| m.modified()).map_err(|e| {
            format!("Error reading file {}: {}.", path.display(), e)
        })?;
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
    let mut config = read_config_file(&bindings_json).unwrap_or_else(|e| {
        panic!("Could not parse {}: {}", bindings_json.display(), e)
    });
    let bindings_cpp = out_dir.join(&config.cpp_file);
    let mut bindings_h = bindings_cpp.clone();
    bindings_h.set_extension("h");
    config.cpp_file = bindings_cpp.clone();
    config.rust.dir = out_dir.join(&config.rust.dir);
    let interface_rs = get_interface_module_path(&config);
    if should_run(
        &[&bindings_json],
        &[&bindings_h, &bindings_cpp, &interface_rs],
    )
    {
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
