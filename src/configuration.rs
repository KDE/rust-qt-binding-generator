use serde_json;
use std::collections::{BTreeMap, BTreeSet};
use std::error::Error;
use std::fs;
use std::path::{Path, PathBuf};
use std::rc::Rc;

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

pub struct Config {
    pub config_file: PathBuf,
    pub cpp_file: PathBuf,
    pub objects: BTreeMap<String, Rc<Object>>,
    pub rust: Rust,
    pub overwrite_implementation: bool,
}

impl Config {
    pub fn types(&self) -> BTreeSet<String> {
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
    pub fn optional_types(&self) -> BTreeSet<String> {
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
    pub fn has_list_or_tree(&self) -> bool {
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
    pub column_count: usize,
}

impl Object {
    pub fn contains_object(&self) -> bool {
        self.properties.values().any(|p| p.is_object())
    }
}

#[derive(PartialEq)]
pub struct Property {
    pub optional: bool,
    pub property_type: Type,
    pub rust_by_function: bool,
    pub write: bool,
}

impl Property {
    pub fn is_object(&self) -> bool {
        self.property_type.is_object()
    }
    pub fn is_complex(&self) -> bool {
        self.property_type.is_complex()
    }
    pub fn type_name(&self) -> &str {
        self.property_type.name()
    }
    pub fn c_get_type(&self) -> String {
        let name = self.property_type.name();
        name.to_string() + "*, " + &name.to_lowercase() + "_set"
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

#[derive(Deserialize, Clone, Copy, PartialEq)]
pub enum ObjectType {
    Object,
    List,
    Tree,
}

#[derive(Deserialize, Clone, Copy, PartialEq)]
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

impl SimpleType {
    pub fn name(&self) -> &str {
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
    pub fn cpp_set_type(&self) -> &str {
        match self {
            SimpleType::QString => "const QString&",
            SimpleType::QByteArray => "const QByteArray&",
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
    pub fn c_set_type(&self) -> &str {
        match self {
            SimpleType::QString => "qstring_t",
            SimpleType::QByteArray => "qbytearray_t",
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
    pub fn rust_type(&self) -> &str {
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
    pub fn rust_type_init(&self) -> &str {
        match self {
            SimpleType::QString => "String::new()",
            SimpleType::QByteArray => "Vec::new()",
            SimpleType::Bool => "false",
            SimpleType::Float => "0.0",
            SimpleType::Double => "0.0",
            SimpleType::Void => "()",
            SimpleType::Qint8 => "0",
            SimpleType::Qint16 => "0",
            SimpleType::Qint32 => "0",
            SimpleType::Qint64 => "0",
            SimpleType::QUint8 => "0",
            SimpleType::QUint16 => "0",
            SimpleType::QUint32 => "0",
            SimpleType::QUint64 => "0",
        }
    }
    pub fn is_complex(self) -> bool {
        self == SimpleType::QString || self == SimpleType::QByteArray
    }
}

#[derive(PartialEq)]
pub enum Type {
    Simple(SimpleType),
    Object(Rc<Object>),
}

impl Type {
    pub fn is_object(&self) -> bool {
        if let Type::Object(_) = self {
            true
        } else {
            false
        }
    }
    pub fn is_complex(&self) -> bool {
        if let Type::Simple(simple) = self {
            simple.is_complex()
        } else {
            false
        }
    }
    pub fn name(&self) -> &str {
        match self {
            Type::Simple(s) => s.name(),
            Type::Object(o) => &o.name,
        }
    }
    pub fn cpp_set_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.cpp_set_type(),
            Type::Object(o) => &o.name,
        }
    }
    pub fn c_set_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.c_set_type(),
            Type::Object(o) => &o.name,
        }
    }
    pub fn rust_type(&self) -> &str {
        match self {
            Type::Simple(s) => s.rust_type(),
            Type::Object(o) => &o.name,
        }
    }
    pub fn rust_type_init(&self) -> &str {
        match self {
            Type::Simple(s) => s.rust_type_init(),
            Type::Object(_) => unimplemented!(),
        }
    }
}

#[derive(Deserialize, Clone, PartialEq)]
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

impl ItemProperty {
    pub fn type_name(&self) -> &str {
        self.item_property_type.name()
    }
    pub fn is_complex(&self) -> bool {
        self.item_property_type.is_complex()
    }
    pub fn cpp_set_type(&self) -> String {
        let mut t = self.item_property_type.cpp_set_type().to_string();
        if self.optional {
            t = "option_".to_string() + &t;
        }
        t
    }
    pub fn c_get_type(&self) -> String {
        let name = self.item_property_type.name();
        name.to_string() + "*, " + &name.to_lowercase() + "_set"
    }
    pub fn c_set_type(&self) -> &str {
        self.item_property_type.c_set_type()
    }
}

#[derive(Deserialize, Clone, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct Function {
    #[serde(rename = "return")]
    pub return_type: SimpleType,
    #[serde(rename = "mut", default = "json::false_bool")]
    pub mutable: bool,
    #[serde(default)]
    pub arguments: Vec<Argument>,
}

impl Function {
    pub fn type_name(&self) -> &str {
        self.return_type.name()
    }
}

#[derive(Deserialize, Clone, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct Argument {
    pub name: String,
    #[serde(rename = "type")]
    pub argument_type: SimpleType,
}

impl Argument {
    pub fn type_name(&self) -> &str {
        self.argument_type.name()
    }
}

fn post_process_property(
    a: (&String, &json::Property),
    b: &mut BTreeMap<String, Rc<Object>>,
    c: &BTreeMap<String, json::Object>,
) -> Result<Property, Box<Error>> {
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
) -> Result<(), Box<Error>> {
    let mut properties = BTreeMap::default();
    for p in &a.1.properties {
        properties.insert(p.0.clone(), post_process_property(p, b, c)?);
    }
    let mut column_count = 1;
    for ip in &a.1.item_properties {
        column_count = column_count.max(ip.1.roles.len());
    }
    let object = Rc::new(Object {
        name: a.0.clone(),
        object_type: a.1.object_type,
        functions: a.1.functions.clone(),
        item_properties: a.1.item_properties.clone(),
        properties,
        column_count,
    });
    b.insert(a.0.clone(), object);
    Ok(())
}

fn post_process(config_file: &Path, json: json::Config) -> Result<Config, Box<Error>> {
    let mut objects = BTreeMap::default();
    for object in &json.objects {
        post_process_object(object, &mut objects, &json.objects)?;
    }
    Ok(Config {
        config_file: config_file.into(),
        cpp_file: json.cpp_file,
        objects,
        rust: json.rust,
        overwrite_implementation: json.overwrite_implementation,
    })
}

pub fn parse<P: AsRef<Path>>(config_file: P) -> Result<Config, Box<Error>> {
    let contents = fs::read_to_string(config_file.as_ref())?;
    let config: json::Config = serde_json::from_str(&contents)?;
    post_process(config_file.as_ref(), config)
}
