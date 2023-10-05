use configuration_private::*;
use serde_json;
use std::collections::{BTreeMap, BTreeSet};
use std::error::Error;
use std::fs;
use std::path::{Path, PathBuf};
use std::rc::Rc;
use toml;

mod json {
    use super::Rust;
    use std::collections::BTreeMap;
    use std::path::PathBuf;

    pub fn false_bool() -> bool {
        false
    }

    fn object() -> super::ObjectType {
        super::ObjectType::Object
    }

    #[derive(Deserialize)]
    #[serde(deny_unknown_fields)]
    pub struct Config {
        #[serde(rename = "cppFile")]
        pub cpp_file: PathBuf,
        pub objects: BTreeMap<String, Object>,
        pub rust: Rust,
        #[serde(default = "false_bool")]
        pub overwrite_implementation: bool,
    }

    #[derive(Deserialize)]
    #[serde(deny_unknown_fields)]
    pub struct Object {
        #[serde(default)]
        pub functions: BTreeMap<String, super::Function>,
        #[serde(rename = "itemProperties", default)]
        pub item_properties: BTreeMap<String, super::ItemProperty>,
        #[serde(rename = "type", default = "object")]
        pub object_type: super::ObjectType,
        #[serde(default)]
        pub properties: BTreeMap<String, Property>,
    }

    #[derive(Deserialize)]
    #[serde(deny_unknown_fields)]
    pub struct Property {
        #[serde(default = "false_bool")]
        pub optional: bool,
        #[serde(rename = "type")]
        pub property_type: String,
        #[serde(rename = "rustByFunction", default = "false_bool")]
        pub rust_by_function: bool,
        #[serde(default = "false_bool")]
        pub write: bool,
    }
}

pub enum RustEdition {
    Rust2015,
    Rust2018,
    Rust2021,
    Unknown,
}

impl<'a> ::std::convert::From<Option<&'a str>> for RustEdition {
    fn from(str: Option<&'a str>) -> RustEdition {
        match str {
            None | Some("2015") => RustEdition::Rust2015,
            Some("2018") => RustEdition::Rust2018,
            Some("2021") => RustEdition::Rust2021,
            _ => RustEdition::Unknown,
        }
    }
}

pub struct Config {
    pub config_file: PathBuf,
    pub cpp_file: PathBuf,
    pub objects: BTreeMap<String, Rc<Object>>,
    pub rust: Rust,
    pub rust_edition: RustEdition,
    pub overwrite_implementation: bool,
}

impl ConfigPrivate for Config {
    fn types(&self) -> BTreeSet<String> {
        let mut ops = BTreeSet::new();
        for o in self.objects.values() {
            for p in o.properties.values() {
                ops.insert(p.type_name().into());
            }
            for p in o.item_properties.values() {
                ops.insert(p.type_name().into());
            }
            for f in o.functions.values() {
                ops.insert(f.return_type.name().into());
                for a in &f.arguments {
                    ops.insert(a.type_name().into());
                }
            }
        }
        ops
    }
    fn optional_types(&self) -> BTreeSet<String> {
        let mut ops = BTreeSet::new();
        for o in self.objects.values() {
            for p in o.properties.values() {
                if p.optional {
                    ops.insert(p.type_name().into());
                }
            }
            for p in o.item_properties.values() {
                if p.optional {
                    ops.insert(p.type_name().into());
                }
            }
            if o.object_type != ObjectType::Object {
                ops.insert("quintptr".into());
            }
        }
        ops
    }
    fn has_list_or_tree(&self) -> bool {
        self.objects
            .values()
            .any(|o| o.object_type == ObjectType::List || o.object_type == ObjectType::Tree)
    }
}

#[derive(PartialEq)]
pub struct Object {
    pub name: String,
    pub functions: BTreeMap<String, Function>,
    pub item_properties: BTreeMap<String, ItemProperty>,
    pub object_type: ObjectType,
    pub properties: BTreeMap<String, Property>,
}

impl ObjectPrivate for Object {
    fn contains_object(&self) -> bool {
        self.properties.values().any(|p| p.is_object())
    }
    fn column_count(&self) -> usize {
        let mut column_count = 1;
        for ip in self.item_properties.values() {
            column_count = column_count.max(ip.roles.len());
        }
        column_count
    }
}

#[derive(PartialEq)]
pub struct Property {
    pub optional: bool,
    pub property_type: Type,
    pub rust_by_function: bool,
    pub write: bool,
}

impl PropertyPrivate for Property {
    fn is_object(&self) -> bool {
        self.property_type.is_object()
    }
    fn is_complex(&self) -> bool {
        self.property_type.is_complex()
    }
    fn c_get_type(&self) -> String {
        let name = self.property_type.name();
        name.to_string() + "*, " + &name.to_lowercase() + "_set"
    }
}

impl TypeName for Property {
    fn type_name(&self) -> &str {
        self.property_type.name()
    }
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Rust {
    pub dir: PathBuf,
    #[serde(rename = "implementationModule")]
    pub implementation_module: String,
    #[serde(rename = "interfaceModule")]
    pub interface_module: String,
}

#[derive(Deserialize, Clone, Copy, PartialEq, Eq)]
pub enum ObjectType {
    Object,
    List,
    Tree,
}

#[derive(Deserialize, Clone, Copy, PartialEq, Eq)]
pub enum SimpleType {
    QString,
    QByteArray,
    #[serde(rename = "bool")]
    Bool,
    #[serde(rename = "float")]
    Float,
    #[serde(rename = "double")]
    Double,
    #[serde(rename = "void")]
    Void,
    #[serde(rename = "qint8")]
    Qint8,
    #[serde(rename = "qint16")]
    Qint16,
    #[serde(rename = "qint32")]
    Qint32,
    #[serde(rename = "qint64")]
    Qint64,
    #[serde(rename = "quint8")]
    QUint8,
    #[serde(rename = "quint16")]
    QUint16,
    #[serde(rename = "quint32")]
    QUint32,
    #[serde(rename = "quint64")]
    QUint64,
}

impl SimpleTypePrivate for SimpleType {
    fn name(&self) -> &str {
        match self {
            SimpleType::QString => "QString",
            SimpleType::QByteArray => "QByteArray",
            SimpleType::Bool => "bool",
            SimpleType::Float => "float",
            SimpleType::Double => "double",
            SimpleType::Void => "void",
            SimpleType::Qint8 => "qint8",
            SimpleType::Qint16 => "qint16",
            SimpleType::Qint32 => "qint32",
            SimpleType::Qint64 => "qint64",
            SimpleType::QUint8 => "quint8",
            SimpleType::QUint16 => "quint16",
            SimpleType::QUint32 => "quint32",
            SimpleType::QUint64 => "quint64",
        }
    }
    fn cpp_set_type(&self) -> &str {
        match self {
            SimpleType::QString => "const QString&",
            SimpleType::QByteArray => "const QByteArray&",
            _ => self.name(),
        }
    }
    fn c_set_type(&self) -> &str {
        match self {
            SimpleType::QString => "qstring_t",
            SimpleType::QByteArray => "qbytearray_t",
            _ => self.name(),
        }
    }
    fn rust_type(&self) -> &str {
        match self {
            SimpleType::QString => "String",
            SimpleType::QByteArray => "Vec<u8>",
            SimpleType::Bool => "bool",
            SimpleType::Float => "f32",
            SimpleType::Double => "f64",
            SimpleType::Void => "()",
            SimpleType::Qint8 => "i8",
            SimpleType::Qint16 => "i16",
            SimpleType::Qint32 => "i32",
            SimpleType::Qint64 => "i64",
            SimpleType::QUint8 => "u8",
            SimpleType::QUint16 => "u16",
            SimpleType::QUint32 => "u32",
            SimpleType::QUint64 => "u64",
        }
    }
    fn rust_type_init(&self) -> &str {
        match self {
            SimpleType::QString => "String::new()",
            SimpleType::QByteArray => "Vec::new()",
            SimpleType::Bool => "false",
            SimpleType::Float | SimpleType::Double => "0.0",
            SimpleType::Void => "()",
            _ => "0",
        }
    }
    fn is_complex(&self) -> bool {
        self == &SimpleType::QString || self == &SimpleType::QByteArray
    }
}

#[derive(PartialEq)]
pub enum Type {
    Simple(SimpleType),
    Object(Rc<Object>),
}

impl TypePrivate for Type {
    fn is_object(&self) -> bool {
        matches!(self, Type::Object(_))
    }
    fn is_complex(&self) -> bool {
        match self {
            Type::Simple(simple) => simple.is_complex(),
            _ => false,
        }
    }
    fn name(&self) -> &str {
        match self {
            Type::Simple(s) => s.name(),
            Type::Object(o) => &o.name,
        }
    }
    fn cpp_set_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.cpp_set_type(),
            Type::Object(o) => &o.name,
        }
    }
    fn c_set_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.c_set_type(),
            Type::Object(o) => &o.name,
        }
    }
    fn rust_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.rust_type(),
            Type::Object(o) => &o.name,
        }
    }
    fn rust_type_init(&self) -> &str {
        match self {
            Type::Simple(s) => s.rust_type_init(),
            Type::Object(_) => unimplemented!(),
        }
    }
}

#[derive(Deserialize, Clone, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct ItemProperty {
    #[serde(rename = "type")]
    pub item_property_type: SimpleType,
    #[serde(default = "json::false_bool")]
    pub optional: bool,
    #[serde(default)]
    pub roles: Vec<Vec<String>>,
    #[serde(rename = "rustByValue", default = "json::false_bool")]
    pub rust_by_value: bool,
    #[serde(default = "json::false_bool")]
    pub write: bool,
}

impl TypeName for ItemProperty {
    fn type_name(&self) -> &str {
        self.item_property_type.name()
    }
}

impl ItemPropertyPrivate for ItemProperty {
    fn is_complex(&self) -> bool {
        self.item_property_type.is_complex()
    }
    fn cpp_set_type(&self) -> String {
        let t = self.item_property_type.cpp_set_type().to_string();
        if self.optional {
            return "option_".to_string() + &t;
        }
        t
    }
    fn c_get_type(&self) -> String {
        let name = self.item_property_type.name();
        name.to_string() + "*, " + &name.to_lowercase() + "_set"
    }
    fn c_set_type(&self) -> &str {
        self.item_property_type.c_set_type()
    }
}

#[derive(Deserialize, Clone, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Function {
    #[serde(rename = "return")]
    pub return_type: SimpleType,
    #[serde(rename = "mut", default = "json::false_bool")]
    pub mutable: bool,
    #[serde(default)]
    pub arguments: Vec<Argument>,
}

impl TypeName for Function {
    fn type_name(&self) -> &str {
        self.return_type.name()
    }
}

#[derive(Deserialize, Clone, PartialEq, Eq)]
#[serde(deny_unknown_fields)]
pub struct Argument {
    pub name: String,
    #[serde(rename = "type")]
    pub argument_type: SimpleType,
}

impl TypeName for Argument {
    fn type_name(&self) -> &str {
        self.argument_type.name()
    }
}

fn post_process_property(
    a: (&String, &json::Property),
    b: &mut BTreeMap<String, Rc<Object>>,
    c: &BTreeMap<String, json::Object>,
) -> Result<Property, Box<dyn Error>> {
    let name = &a.1.property_type;
    let t = match serde_json::from_str::<SimpleType>(&format!("\"{}\"", name)) {
        Err(_) => {
            if b.get(name).is_none() {
                if let Some(object) = c.get(name) {
                    post_process_object((name, object), b, c)?;
                } else {
                    return Err(format!("Type {} cannot be found.", name).into());
                }
            }
            Type::Object(Rc::clone(b.get(name).unwrap()))
        }
        Ok(simple) => Type::Simple(simple),
    };
    Ok(Property {
        property_type: t,
        optional: a.1.optional,
        rust_by_function: a.1.rust_by_function,
        write: a.1.write,
    })
}

fn post_process_object(
    a: (&String, &json::Object),
    b: &mut BTreeMap<String, Rc<Object>>,
    c: &BTreeMap<String, json::Object>,
) -> Result<(), Box<dyn Error>> {
    let mut properties = BTreeMap::default();
    for p in &a.1.properties {
        properties.insert(p.0.clone(), post_process_property(p, b, c)?);
    }
    let object = Rc::new(Object {
        name: a.0.clone(),
        object_type: a.1.object_type,
        functions: a.1.functions.clone(),
        item_properties: a.1.item_properties.clone(),
        properties,
    });
    b.insert(a.0.clone(), object);
    Ok(())
}

fn post_process(config_file: &Path, json: json::Config) -> Result<Config, Box<dyn Error>> {
    let mut objects = BTreeMap::default();
    for object in &json.objects {
        post_process_object(object, &mut objects, &json.objects)?;
    }

    let rust_edition: RustEdition = {
        let mut buf = config_file.to_path_buf();
        buf.pop();
        buf.push(&json.rust.dir);
        buf.push("Cargo.toml");
        if !buf.exists() {
            return Err(format!("{} does not exist.", buf.display()).into());
        }
        let manifest: toml::Value = fs::read_to_string(&buf)?.parse()?;
        manifest["package"]
            .get("edition")
            .and_then(|val| val.as_str())
            .into()
    };

    Ok(Config {
        config_file: config_file.into(),
        cpp_file: json.cpp_file,
        objects,
        rust: json.rust,
        rust_edition,
        overwrite_implementation: json.overwrite_implementation,
    })
}

pub fn parse<P: AsRef<Path>>(config_file: P) -> Result<Config, Box<dyn Error>> {
    let contents = fs::read_to_string(config_file.as_ref())?;
    let config: json::Config = serde_json::from_str(&contents)?;
    post_process(config_file.as_ref(), config)
}
