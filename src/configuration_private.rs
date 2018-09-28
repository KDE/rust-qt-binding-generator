use std::collections::BTreeSet;

pub trait ConfigPrivate {
    fn types(&self) -> BTreeSet<String>;
    fn optional_types(&self) -> BTreeSet<String>;
    fn has_list_or_tree(&self) -> bool;
}

pub trait ObjectPrivate {
    fn contains_object(&self) -> bool;
    fn column_count(&self) -> usize;
}

pub trait TypeName {
    fn type_name(&self) -> &str;
}

pub trait TypePrivate {
    fn is_object(&self) -> bool;
    fn is_complex(&self) -> bool;
    fn name(&self) -> &str;
    fn cpp_set_type(&self) -> &str;
    fn c_set_type(&self) -> &str;
    fn rust_type(&self) -> &str;
    fn rust_type_init(&self) -> &str;
}

pub trait PropertyPrivate {
    fn is_object(&self) -> bool;
    fn is_complex(&self) -> bool;
    fn c_get_type(&self) -> String;
}

pub trait SimpleTypePrivate {
    fn name(&self) -> &str;
    fn cpp_set_type(&self) -> &str;
    fn c_set_type(&self) -> &str;
    fn rust_type(&self) -> &str;
    fn rust_type_init(&self) -> &str;
    fn is_complex(self) -> bool;
}

pub trait ItemPropertyPrivate {
    fn is_complex(&self) -> bool;
    fn cpp_set_type(&self) -> String;
    fn c_get_type(&self) -> String;
    fn c_set_type(&self) -> &str;
}
