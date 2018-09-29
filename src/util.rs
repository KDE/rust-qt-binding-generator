use regex::Regex;
use std::fs;
use std::io::Result;
use std::path::Path;

pub fn write_if_different<P: AsRef<Path>>(path: P, contents: &[u8]) -> Result<()> {
    let old_contents = fs::read(&path).ok();
    if old_contents.map(|c| c == contents).unwrap_or(false) {
        Ok(())
    } else {
        let _ = fs::create_dir_all(path.as_ref().parent().unwrap());
        fs::write(path, contents)
    }
}

pub fn snake_case(name: &str) -> String {
    let re = Regex::new("([A-Z])").unwrap();
    (name[..1].to_string() + &re.replace_all(&name[1..], "_$1")).to_lowercase()
}
