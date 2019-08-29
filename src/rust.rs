//! `rust` is the module that generates the rust code for the binding

use configuration::*;
use configuration_private::*;
use std::io::{Result, Write};
use util::{snake_case, write_if_different};

fn rust_type(p: &Property) -> String {
    if p.optional {
        return format!("Option<{}>", p.property_type.rust_type());
    }
    p.property_type.rust_type().to_string()
}

fn rust_type_(p: &ItemProperty) -> String {
    if p.optional {
        return format!("Option<{}>", p.item_property_type.rust_type());
    }
    p.item_property_type.rust_type().to_string()
}

fn rust_return_type(p: &Property) -> String {
    let mut type_: String = p.property_type.rust_type().to_string();
    if type_ == "String" {
        type_ = "str".to_string();
    }
    if type_ == "Vec<u8>" {
        type_ = "[u8]".to_string();
    }
    if p.property_type.is_complex() {
        type_ = "&".to_string() + &type_;
    }
    if p.optional {
        return "Option<".to_string() + &type_ + ">";
    }
    type_
}

fn rust_return_type_(p: &ItemProperty) -> String {
    let mut type_: String = p.item_property_type.rust_type().to_string();
    if type_ == "String" && !p.rust_by_value {
        type_ = "str".to_string();
    }
    if type_ == "Vec<u8>" && !p.rust_by_value {
        type_ = "[u8]".to_string();
    }
    if p.item_property_type.is_complex() && !p.rust_by_value {
        type_ = "&".to_string() + &type_;
    }
    if p.optional {
        return "Option<".to_string() + &type_ + ">";
    }
    type_
}

fn rust_c_type(p: &ItemProperty) -> String {
    if p.optional {
        return format!("COption<{}>", p.item_property_type.rust_type());
    }
    p.item_property_type.rust_type().to_string()
}

fn rust_type_init(p: &Property) -> &str {
    if p.optional {
        return "None";
    }
    p.property_type.rust_type_init()
}

fn r_constructor_args_decl(r: &mut Vec<u8>, name: &str, o: &Object, conf: &Config) -> Result<()> {
    write!(r, "    {}: *mut {}QObject", snake_case(name), o.name)?;
    for (p_name, p) in &o.properties {
        if let Type::Object(object) = &p.property_type {
            writeln!(r, ",")?;
            r_constructor_args_decl(r, p_name, object, conf)?;
        } else {
            write!(
                r,
                ",\n    {}_{}_changed: fn(*mut {}QObject)",
                snake_case(name),
                snake_case(p_name),
                o.name
            )?;
        }
    }
    if o.object_type == ObjectType::List {
        write!(
            r,
            ",\n    {}_new_data_ready: fn(*mut {}QObject)",
            snake_case(name),
            o.name
        )?;
    } else if o.object_type == ObjectType::Tree {
        write!(
            r,
            ",\n    {}_new_data_ready: fn(*mut {}QObject, index: COption<usize>)",
            snake_case(name),
            o.name
        )?;
    }
    if o.object_type != ObjectType::Object {
        let index_decl = if o.object_type == ObjectType::Tree {
            " index: COption<usize>,"
        } else {
            ""
        };
        let dest_decl = if o.object_type == ObjectType::Tree {
            " index: COption<usize>,"
        } else {
            ""
        };
        write!(
            r,
            ",
    {2}_layout_about_to_be_changed: fn(*mut {0}QObject),
    {2}_layout_changed: fn(*mut {0}QObject),
    {2}_data_changed: fn(*mut {0}QObject, usize, usize),
    {2}_begin_reset_model: fn(*mut {0}QObject),
    {2}_end_reset_model: fn(*mut {0}QObject),
    {2}_begin_insert_rows: fn(*mut {0}QObject,{1} usize, usize),
    {2}_end_insert_rows: fn(*mut {0}QObject),
    {2}_begin_move_rows: fn(*mut {0}QObject,{1} usize, usize,{3} usize),
    {2}_end_move_rows: fn(*mut {0}QObject),
    {2}_begin_remove_rows: fn(*mut {0}QObject,{1} usize, usize),
    {2}_end_remove_rows: fn(*mut {0}QObject)",
            o.name,
            index_decl,
            snake_case(name),
            dest_decl
        )?;
    }
    Ok(())
}

fn r_constructor_args(r: &mut Vec<u8>, name: &str, o: &Object, conf: &Config) -> Result<()> {
    for (name, p) in &o.properties {
        if let Type::Object(object) = &p.property_type {
            r_constructor_args(r, name, object, conf)?;
        }
    }
    writeln!(
        r,
        "    let {}_emit = {}Emitter {{
        qobject: Arc::new(AtomicPtr::new({0})),",
        snake_case(name),
        o.name
    )?;
    for (p_name, p) in &o.properties {
        if p.is_object() {
            continue;
        }
        writeln!(
            r,
            "        {}_changed: {}_{0}_changed,",
            snake_case(p_name),
            snake_case(name)
        )?;
    }
    if o.object_type != ObjectType::Object {
        writeln!(
            r,
            "        new_data_ready: {}_new_data_ready,",
            snake_case(name)
        )?;
    }
    let mut model = String::new();
    if o.object_type != ObjectType::Object {
        let type_ = if o.object_type == ObjectType::List {
            "List"
        } else {
            "Tree"
        };
        model.push_str(", model");
        writeln!(
            r,
            "    }};
    let model = {}{} {{
        qobject: {},
        layout_about_to_be_changed: {2}_layout_about_to_be_changed,
        layout_changed: {2}_layout_changed,
        data_changed: {2}_data_changed,
        begin_reset_model: {2}_begin_reset_model,
        end_reset_model: {2}_end_reset_model,
        begin_insert_rows: {2}_begin_insert_rows,
        end_insert_rows: {2}_end_insert_rows,
        begin_move_rows: {2}_begin_move_rows,
        end_move_rows: {2}_end_move_rows,
        begin_remove_rows: {2}_begin_remove_rows,
        end_remove_rows: {2}_end_remove_rows,",
            o.name,
            type_,
            snake_case(name)
        )?;
    }
    write!(
        r,
        "    }};\n    let d_{} = {}::new({0}_emit{}",
        snake_case(name),
        o.name,
        model
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            write!(r, ",\n        d_{}", snake_case(name))?;
        }
    }
    writeln!(r, ");")
}

fn write_function(
    r: &mut Vec<u8>,
    (name, f): (&String, &Function),
    lcname: &str,
    o: &Object,
) -> Result<()> {
    let lc = snake_case(name);
    write!(
        r,
        "
#[no_mangle]
pub unsafe extern \"C\" fn {}_{}(ptr: *{} {}",
        lcname,
        lc,
        if f.mutable { "mut" } else { "const" },
        o.name
    )?;
    // write all the input arguments, for QString and QByteArray, write
    // pointers to their content and the length which is int in Qt
    for a in &f.arguments {
        write!(r, ", ")?;
        if a.argument_type.name() == "QString" {
            write!(r, "{}_str: *const c_ushort, {0}_len: c_int", a.name)?;
        } else if a.argument_type.name() == "QByteArray" {
            write!(r, "{}_str: *const c_char, {0}_len: c_int", a.name)?;
        } else {
            write!(r, "{}: {}", a.name, a.argument_type.rust_type())?;
        }
    }
    // If the return type is QString or QByteArray, append a pointer to the
    // variable that will be set to the argument list. Also add a setter
    // function.
    if f.return_type.is_complex() {
        writeln!(
            r,
            ", d: *mut {}, set: fn(*mut {0}, str: *const c_char, len: c_int)) {{",
            f.return_type.name()
        )?;
    } else if f.return_type == SimpleType::Void {
        writeln!(r, ") {{")?;
    } else {
        writeln!(r, ") -> {} {{", f.return_type.rust_type())?;
    }
    for a in &f.arguments {
        if a.argument_type.name() == "QString" {
            writeln!(
                r,
                "    let mut {} = String::new();
    set_string_from_utf16(&mut {0}, {0}_str, {0}_len);",
                a.name
            )?;
        } else if a.argument_type.name() == "QByteArray" {
            writeln!(
                r,
                "    let {} = {{ slice::from_raw_parts({0}_str as *const u8, to_usize({0}_len)) }};",
                a.name
            )?;
        }
    }
    if f.mutable {
        writeln!(r, "    let o = &mut *ptr;")?;
    } else {
        writeln!(r, "    let o = &*ptr;")?;
    }
    if f.return_type.is_complex() {
        write!(r, "    let r = o.{}(", lc)?;
    } else {
        write!(r, "    o.{}(", lc)?;
    }
    for (i, a) in f.arguments.iter().enumerate() {
        if i > 0 {
            write!(r, ", ")?;
        }
        write!(r, "{}", a.name)?;
    }
    write!(r, ")")?;
    if f.return_type.is_complex() {
        writeln!(r, ";")?;
        writeln!(
            r,
            "    let s: *const c_char = r.as_ptr() as (*const c_char);
    set(d, s, r.len() as i32);"
        )?;
    } else {
        writeln!(r)?;
    }
    writeln!(r, "}}")
}

fn write_rust_interface_object(r: &mut Vec<u8>, o: &Object, conf: &Config) -> Result<()> {
    let lcname = snake_case(&o.name);
    writeln!(
        r,
        "
pub struct {}QObject {{}}

pub struct {0}Emitter {{
    qobject: Arc<AtomicPtr<{0}QObject>>,",
        o.name
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            continue;
        }
        writeln!(
            r,
            "    {}_changed: fn(*mut {}QObject),",
            snake_case(name),
            o.name
        )?;
    }
    if o.object_type == ObjectType::List {
        writeln!(r, "    new_data_ready: fn(*mut {}QObject),", o.name)?;
    } else if o.object_type == ObjectType::Tree {
        writeln!(
            r,
            "    new_data_ready: fn(*mut {}QObject, index: COption<usize>),",
            o.name
        )?;
    }
    writeln!(
        r,
        "}}

unsafe impl Send for {}Emitter {{}}

impl {0}Emitter {{
    /// Clone the emitter
    ///
    /// The emitter can only be cloned when it is mutable. The emitter calls
    /// into C++ code which may call into Rust again. If emmitting is possible
    /// from immutable structures, that might lead to access to a mutable
    /// reference. That is undefined behaviour and forbidden.
    pub fn clone(&mut self) -> {0}Emitter {{
        {0}Emitter {{
            qobject: self.qobject.clone(),",
        o.name
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            continue;
        }
        writeln!(
            r,
            "            {}_changed: self.{0}_changed,",
            snake_case(name),
        )?;
    }
    if o.object_type != ObjectType::Object {
        writeln!(r, "            new_data_ready: self.new_data_ready,")?;
    }
    writeln!(
        r,
        "        }}
    }}
    fn clear(&self) {{
        let n: *const {0}QObject = null();
        self.qobject.store(n as *mut {0}QObject, Ordering::SeqCst);
    }}",
        o.name
    )?;

    for (name, p) in &o.properties {
        if p.is_object() {
            continue;
        }
        writeln!(
            r,
            "    pub fn {}_changed(&mut self) {{
        let ptr = self.qobject.load(Ordering::SeqCst);
        if !ptr.is_null() {{
            (self.{0}_changed)(ptr);
        }}
    }}",
            snake_case(name)
        )?;
    }

    if o.object_type == ObjectType::List {
        writeln!(
            r,
            "    pub fn new_data_ready(&mut self) {{
        let ptr = self.qobject.load(Ordering::SeqCst);
        if !ptr.is_null() {{
            (self.new_data_ready)(ptr);
        }}
    }}"
        )?;
    } else if o.object_type == ObjectType::Tree {
        writeln!(
            r,
            "    pub fn new_data_ready(&mut self, item: Option<usize>) {{
        let ptr = self.qobject.load(Ordering::SeqCst);
        if !ptr.is_null() {{
            (self.new_data_ready)(ptr, item.into());
        }}
    }}"
        )?;
    }

    let mut model_struct = String::new();
    if o.object_type != ObjectType::Object {
        let type_ = if o.object_type == ObjectType::List {
            "List"
        } else {
            "Tree"
        };
        model_struct = format!(", model: {}{}", o.name, type_);
        let mut index = "";
        let mut index_decl = "";
        let mut index_c_decl = "";
        let mut dest = "";
        let mut dest_decl = "";
        let mut dest_c_decl = "";
        if o.object_type == ObjectType::Tree {
            index_decl = " index: Option<usize>,";
            index_c_decl = " index: COption<usize>,";
            index = " index.into(),";
            dest_decl = " dest: Option<usize>,";
            dest_c_decl = " dest: COption<usize>,";
            dest = " dest.into(),";
        }
        writeln!(
            r,
            "}}

#[derive(Clone)]
pub struct {0}{1} {{
    qobject: *mut {0}QObject,
    layout_about_to_be_changed: fn(*mut {0}QObject),
    layout_changed: fn(*mut {0}QObject),
    data_changed: fn(*mut {0}QObject, usize, usize),
    begin_reset_model: fn(*mut {0}QObject),
    end_reset_model: fn(*mut {0}QObject),
    begin_insert_rows: fn(*mut {0}QObject,{4} usize, usize),
    end_insert_rows: fn(*mut {0}QObject),
    begin_move_rows: fn(*mut {0}QObject,{4} usize, usize,{7} usize),
    end_move_rows: fn(*mut {0}QObject),
    begin_remove_rows: fn(*mut {0}QObject,{4} usize, usize),
    end_remove_rows: fn(*mut {0}QObject),
}}

impl {0}{1} {{
    pub fn layout_about_to_be_changed(&mut self) {{
        (self.layout_about_to_be_changed)(self.qobject);
    }}
    pub fn layout_changed(&mut self) {{
        (self.layout_changed)(self.qobject);
    }}
    pub fn data_changed(&mut self, first: usize, last: usize) {{
        (self.data_changed)(self.qobject, first, last);
    }}
    pub fn begin_reset_model(&mut self) {{
        (self.begin_reset_model)(self.qobject);
    }}
    pub fn end_reset_model(&mut self) {{
        (self.end_reset_model)(self.qobject);
    }}
    pub fn begin_insert_rows(&mut self,{2} first: usize, last: usize) {{
        (self.begin_insert_rows)(self.qobject,{3} first, last);
    }}
    pub fn end_insert_rows(&mut self) {{
        (self.end_insert_rows)(self.qobject);
    }}
    pub fn begin_move_rows(&mut self,{2} first: usize, last: usize,{5} destination: usize) {{
        (self.begin_move_rows)(self.qobject,{3} first, last,{6} destination);
    }}
    pub fn end_move_rows(&mut self) {{
        (self.end_move_rows)(self.qobject);
    }}
    pub fn begin_remove_rows(&mut self,{2} first: usize, last: usize) {{
        (self.begin_remove_rows)(self.qobject,{3} first, last);
    }}
    pub fn end_remove_rows(&mut self) {{
        (self.end_remove_rows)(self.qobject);
    }}",
            o.name, type_, index_decl, index, index_c_decl, dest_decl, dest, dest_c_decl
        )?;
    }

    write!(
        r,
        "}}

pub trait {}Trait {{
    fn new(emit: {0}Emitter{}",
        o.name, model_struct
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            write!(r, ",\n        {}: {}", snake_case(name), p.type_name())?;
        }
    }
    writeln!(
        r,
        ") -> Self;
    fn emit(&mut self) -> &mut {}Emitter;",
        o.name
    )?;
    for (name, p) in &o.properties {
        let lc = snake_case(name).to_lowercase();
        if p.is_object() {
            writeln!(r, "    fn {}(&self) -> &{};", lc, rust_type(p))?;
            writeln!(r, "    fn {}_mut(&mut self) -> &mut {};", lc, rust_type(p))?;
        } else {
            if p.rust_by_function {
                write!(
                    r,
                    "    fn {}<F>(&self, getter: F) where F: FnOnce({});",
                    lc,
                    rust_return_type(p)
                )?;
            } else {
                writeln!(r, "    fn {}(&self) -> {};", lc, rust_return_type(p))?;
            }
            if p.write {
                if p.type_name() == "QByteArray" {
                    if p.optional {
                        writeln!(r, "    fn set_{}(&mut self, value: Option<&[u8]>);", lc)?;
                    } else {
                        writeln!(r, "    fn set_{}(&mut self, value: &[u8]);", lc)?;
                    }
                } else {
                    writeln!(r, "    fn set_{}(&mut self, value: {});", lc, rust_type(p))?;
                }
            }
        }
    }
    for (name, f) in &o.functions {
        let lc = snake_case(name);
        let mut arg_list = String::new();
        if !f.arguments.is_empty() {
            for a in &f.arguments {
                let t = if a.argument_type.name() == "QByteArray" {
                    "&[u8]"
                } else {
                    a.argument_type.rust_type()
                };
                arg_list.push_str(&format!(", {}: {}", a.name, t));
            }
        }
        writeln!(
            r,
            "    fn {}(&{}self{}) -> {};",
            lc,
            if f.mutable { "mut " } else { "" },
            arg_list,
            f.return_type.rust_type()
        )?;
    }
    if o.object_type == ObjectType::List {
        writeln!(
            r,
            "    fn row_count(&self) -> usize;
    fn insert_rows(&mut self, _row: usize, _count: usize) -> bool {{ false }}
    fn remove_rows(&mut self, _row: usize, _count: usize) -> bool {{ false }}
    fn can_fetch_more(&self) -> bool {{
        false
    }}
    fn fetch_more(&mut self) {{}}
    fn sort(&mut self, _: u8, _: SortOrder) {{}}"
        )?;
    } else if o.object_type == ObjectType::Tree {
        writeln!(
            r,
            "    fn row_count(&self, _: Option<usize>) -> usize;
    fn can_fetch_more(&self, _: Option<usize>) -> bool {{
        false
    }}
    fn fetch_more(&mut self, _: Option<usize>) {{}}
    fn sort(&mut self, _: u8, _: SortOrder) {{}}
    fn check_row(&self, index: usize, row: usize) -> Option<usize>;
    fn index(&self, item: Option<usize>, row: usize) -> usize;
    fn parent(&self, index: usize) -> Option<usize>;
    fn row(&self, index: usize) -> usize;"
        )?;
    }
    if o.object_type != ObjectType::Object {
        for (name, ip) in &o.item_properties {
            let name = snake_case(name);
            writeln!(
                r,
                "    fn {}(&self, index: usize) -> {};",
                name,
                rust_return_type_(ip)
            )?;
            if ip.write {
                if ip.item_property_type.name() == "QByteArray" {
                    if ip.optional {
                        writeln!(
                            r,
                            "    fn set_{}(&mut self, index: usize, _: Option<&[u8]>) -> bool;",
                            name
                        )?;
                    } else {
                        writeln!(
                            r,
                            "    fn set_{}(&mut self, index: usize, _: &[u8]) -> bool;",
                            name
                        )?;
                    }
                } else {
                    writeln!(
                        r,
                        "    fn set_{}(&mut self, index: usize, _: {}) -> bool;",
                        name,
                        rust_type_(ip)
                    )?;
                }
            }
        }
    }
    writeln!(
        r,
        "}}

#[no_mangle]
pub extern \"C\" fn {}_new(",
        lcname
    )?;
    r_constructor_args_decl(r, &lcname, o, conf)?;
    writeln!(r, ",\n) -> *mut {} {{", o.name)?;
    r_constructor_args(r, &lcname, o, conf)?;
    writeln!(
        r,
        "    Box::into_raw(Box::new(d_{}))
}}

#[no_mangle]
pub unsafe extern \"C\" fn {0}_free(ptr: *mut {}) {{
    Box::from_raw(ptr).emit().clear();
}}",
        lcname, o.name
    )?;

    for (name, p) in &o.properties {
        let base = format!("{}_{}", lcname, snake_case(name));
        if p.is_object() {
            writeln!(
                r,
                "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(ptr: *mut {}) -> *mut {} {{
    (&mut *ptr).{}_mut()
}}",
                base,
                o.name,
                rust_type(p),
                snake_case(name)
            )?;
        } else if p.is_complex() && !p.optional {
            if p.rust_by_function {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(
    ptr: *const {},
    p: *mut {},
    set: fn(*mut {2}, *const c_char, c_int),
) {{
    let o = &*ptr;
    o.{}(|v| {{
        let s: *const c_char = v.as_ptr() as (*const c_char);
        set(p, s, to_c_int(v.len()));
    }});
}}",
                    base,
                    o.name,
                    p.type_name(),
                    snake_case(name)
                )?;
            } else {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(
    ptr: *const {},
    p: *mut {},
    set: fn(*mut {2}, *const c_char, c_int),
) {{
    let o = &*ptr;
    let v = o.{}();
    let s: *const c_char = v.as_ptr() as (*const c_char);
    set(p, s, to_c_int(v.len()));
}}",
                    base,
                    o.name,
                    p.type_name(),
                    snake_case(name)
                )?;
            }
            if p.write && p.type_name() == "QString" {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: *const c_ushort, len: c_int) {{
    let o = &mut *ptr;
    let mut s = String::new();
    set_string_from_utf16(&mut s, v, len);
    o.set_{}(s);
}}",
                    base,
                    o.name,
                    snake_case(name)
                )?;
            } else if p.write {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: *const c_char, len: c_int) {{
    let o = &mut *ptr;
    let v = slice::from_raw_parts(v as *const u8, to_usize(len));
    o.set_{}(v);
}}",
                    base,
                    o.name,
                    snake_case(name)
                )?;
            }
        } else if p.is_complex() {
            writeln!(
                r,
                "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(
    ptr: *const {},
    p: *mut {},
    set: fn(*mut {2}, *const c_char, c_int),
) {{
    let o = &*ptr;
    let v = o.{}();
    if let Some(v) = v {{
        let s: *const c_char = v.as_ptr() as (*const c_char);
        set(p, s, to_c_int(v.len()));
    }}
}}",
                base,
                o.name,
                p.type_name(),
                snake_case(name)
            )?;
            if p.write && p.type_name() == "QString" {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: *const c_ushort, len: c_int) {{
    let o = &mut *ptr;
    let mut s = String::new();
    set_string_from_utf16(&mut s, v, len);
    o.set_{}(Some(s));
}}",
                    base,
                    o.name,
                    snake_case(name)
                )?;
            } else if p.write {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: *const c_char, len: c_int) {{
    let o = &mut *ptr;
    let v = slice::from_raw_parts(v as *const u8, to_usize(len));
    o.set_{}(Some(v.into()));
}}",
                    base,
                    o.name,
                    snake_case(name)
                )?;
            }
        } else if p.optional {
            writeln!(
                r,
                "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(ptr: *const {}) -> COption<{}> {{
    match (&*ptr).{}() {{
        Some(value) => COption {{ data: value, some: true }},
        None => COption {{ data: {2}::default(), some: false}}
    }}
}}",
                base,
                o.name,
                p.property_type.rust_type(),
                snake_case(name)
            )?;
            if p.write {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: {}) {{
    (&mut *ptr).set_{}(Some(v));
}}",
                    base,
                    o.name,
                    p.property_type.rust_type(),
                    snake_case(name)
                )?;
            }
        } else {
            writeln!(
                r,
                "
#[no_mangle]
pub unsafe extern \"C\" fn {}_get(ptr: *const {}) -> {} {{
    (&*ptr).{}()
}}",
                base,
                o.name,
                rust_type(p),
                snake_case(name)
            )?;
            if p.write {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set(ptr: *mut {}, v: {}) {{
    (&mut *ptr).set_{}(v);
}}",
                    base,
                    o.name,
                    rust_type(p),
                    snake_case(name)
                )?;
            }
        }
        if p.write && p.optional {
            writeln!(
                r,
                "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set_none(ptr: *mut {}) {{
    let o = &mut *ptr;
    o.set_{}(None);
}}",
                base,
                o.name,
                snake_case(name)
            )?;
        }
    }
    for f in &o.functions {
        write_function(r, f, &lcname, o)?;
    }
    if o.object_type == ObjectType::List {
        writeln!(
            r,
            "
#[no_mangle]
pub unsafe extern \"C\" fn {1}_row_count(ptr: *const {0}) -> c_int {{
    to_c_int((&*ptr).row_count())
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_insert_rows(ptr: *mut {0}, row: c_int, count: c_int) -> bool {{
    (&mut *ptr).insert_rows(to_usize(row), to_usize(count))
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_remove_rows(ptr: *mut {0}, row: c_int, count: c_int) -> bool {{
    (&mut *ptr).remove_rows(to_usize(row), to_usize(count))
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_can_fetch_more(ptr: *const {0}) -> bool {{
    (&*ptr).can_fetch_more()
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_fetch_more(ptr: *mut {0}) {{
    (&mut *ptr).fetch_more()
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_sort(
    ptr: *mut {0},
    column: u8,
    order: SortOrder,
) {{
    (&mut *ptr).sort(column, order)
}}",
            o.name, lcname
        )?;
    } else if o.object_type == ObjectType::Tree {
        writeln!(
            r,
            "
#[no_mangle]
pub unsafe extern \"C\" fn {1}_row_count(
    ptr: *const {0},
    index: COption<usize>,
) -> c_int {{
    to_c_int((&*ptr).row_count(index.into()))
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_can_fetch_more(
    ptr: *const {0},
    index: COption<usize>,
) -> bool {{
    (&*ptr).can_fetch_more(index.into())
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_fetch_more(ptr: *mut {0}, index: COption<usize>) {{
    (&mut *ptr).fetch_more(index.into())
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_sort(
    ptr: *mut {0},
    column: u8,
    order: SortOrder
) {{
    (&mut *ptr).sort(column, order)
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_check_row(
    ptr: *const {0},
    index: usize,
    row: c_int,
) -> COption<usize> {{
    (&*ptr).check_row(index, to_usize(row)).into()
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_index(
    ptr: *const {0},
    index: COption<usize>,
    row: c_int,
) -> usize {{
    (&*ptr).index(index.into(), to_usize(row))
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_parent(ptr: *const {0}, index: usize) -> QModelIndex {{
    if let Some(parent) = (&*ptr).parent(index) {{
        QModelIndex {{
            row: to_c_int((&*ptr).row(parent)),
            internal_id: parent,
        }}
    }} else {{
        QModelIndex {{
            row: -1,
            internal_id: 0,
        }}
    }}
}}
#[no_mangle]
pub unsafe extern \"C\" fn {1}_row(ptr: *const {0}, index: usize) -> c_int {{
    to_c_int((&*ptr).row(index))
}}",
            o.name, lcname
        )?;
    }
    if o.object_type != ObjectType::Object {
        let (index_decl, index) = if o.object_type == ObjectType::Tree {
            (", index: usize", "index")
        } else {
            (", row: c_int", "to_usize(row)")
        };
        for (name, ip) in &o.item_properties {
            if ip.is_complex() && !ip.optional {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_data_{}(
    ptr: *const {}{},
    d: *mut {},
    set: fn(*mut {4}, *const c_char, len: c_int),
) {{
    let o = &*ptr;
    let data = o.{1}({});
    let s: *const c_char = data.as_ptr() as (*const c_char);
    set(d, s, to_c_int(data.len()));
}}",
                    lcname,
                    snake_case(name),
                    o.name,
                    index_decl,
                    ip.type_name(),
                    index
                )?;
            } else if ip.is_complex() {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_data_{}(
    ptr: *const {}{},
    d: *mut {},
    set: fn(*mut {4}, *const c_char, len: c_int),
) {{
    let o = &*ptr;
    let data = o.{1}({});
    if let Some(data) = data {{
        let s: *const c_char = data.as_ptr() as (*const c_char);
        set(d, s, to_c_int(data.len()));
    }}
}}",
                    lcname,
                    snake_case(name),
                    o.name,
                    index_decl,
                    ip.type_name(),
                    index
                )?;
            } else {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_data_{}(ptr: *const {}{}) -> {} {{
    let o = &*ptr;
    o.{1}({}){}
}}",
                    lcname,
                    snake_case(name),
                    o.name,
                    index_decl,
                    rust_c_type(ip),
                    index,
                    if ip.optional { ".into()" } else { "" }
                )?;
            }
            if ip.write {
                let val = if ip.optional { "Some(v)" } else { "v" };
                if ip.type_name() == "QString" {
                    writeln!(
                        r,
                        "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set_data_{}(
    ptr: *mut {}{},
    s: *const c_ushort, len: c_int,
) -> bool {{
    let o = &mut *ptr;
    let mut v = String::new();
    set_string_from_utf16(&mut v, s, len);
    o.set_{1}({}, {})
}}",
                        lcname,
                        snake_case(name),
                        o.name,
                        index_decl,
                        index,
                        val
                    )?;
                } else if ip.type_name() == "QByteArray" {
                    writeln!(
                        r,
                        "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set_data_{}(
    ptr: *mut {}{},
    s: *const c_char, len: c_int,
) -> bool {{
    let o = &mut *ptr;
    let slice = ::std::slice::from_raw_parts(s as *const u8, to_usize(len));
    o.set_{1}({}, {})
}}",
                        lcname,
                        snake_case(name),
                        o.name,
                        index_decl,
                        index,
                        if ip.optional { "Some(slice)" } else { "slice" }
                    )?;
                } else {
                    let type_ = ip.item_property_type.rust_type();
                    writeln!(
                        r,
                        "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set_data_{}(
    ptr: *mut {}{},
    v: {},
) -> bool {{
    (&mut *ptr).set_{1}({}, {})
}}",
                        lcname,
                        snake_case(name),
                        o.name,
                        index_decl,
                        type_,
                        index,
                        val
                    )?;
                }
            }
            if ip.write && ip.optional {
                writeln!(
                    r,
                    "
#[no_mangle]
pub unsafe extern \"C\" fn {}_set_data_{}_none(ptr: *mut {}{}) -> bool {{
    (&mut *ptr).set_{1}({}, None)
}}",
                    lcname,
                    snake_case(name),
                    o.name,
                    index_decl,
                    index
                )?;
            }
        }
    }
    Ok(())
}

fn write_rust_types(conf: &Config, r: &mut Vec<u8>) -> Result<()> {
    let mut has_option = false;
    let mut has_string = false;
    let mut has_byte_array = false;
    let mut has_list_or_tree = false;

    for o in conf.objects.values() {
        has_list_or_tree |= o.object_type != ObjectType::Object;
        for p in o.properties.values() {
            has_option |= p.optional;
            has_string |= p.property_type == Type::Simple(SimpleType::QString);
            has_byte_array |= p.property_type == Type::Simple(SimpleType::QByteArray);
        }
        for p in o.item_properties.values() {
            has_option |= p.optional;
            has_string |= p.item_property_type == SimpleType::QString;
            has_byte_array |= p.item_property_type == SimpleType::QByteArray;
        }
        for f in o.functions.values() {
            has_string |= f.return_type == SimpleType::QString;
            has_byte_array |= f.return_type == SimpleType::QByteArray;
            for a in &f.arguments {
                has_string |= a.argument_type == SimpleType::QString;
                has_byte_array |= a.argument_type == SimpleType::QByteArray;
            }
        }
    }

    if has_option || has_list_or_tree {
        writeln!(
            r,
            "

#[repr(C)]
pub struct COption<T> {{
    data: T,
    some: bool,
}}

impl<T> COption<T> {{
    #![allow(dead_code)]
    fn into(self) -> Option<T> {{
        if self.some {{
            Some(self.data)
        }} else {{
            None
        }}
    }}
}}

impl<T> From<Option<T>> for COption<T>
where
    T: Default,
{{
    fn from(t: Option<T>) -> COption<T> {{
        if let Some(v) = t {{
            COption {{
                data: v,
                some: true,
            }}
        }} else {{
            COption {{
                data: T::default(),
                some: false,
            }}
        }}
    }}
}}"
        )?;
    }
    if has_string {
        writeln!(
            r,
            "

pub enum QString {{}}

fn set_string_from_utf16(s: &mut String, str: *const c_ushort, len: c_int) {{
    let utf16 = unsafe {{ slice::from_raw_parts(str, to_usize(len)) }};
    let characters = decode_utf16(utf16.iter().cloned())
        .map(|r| r.unwrap());
    s.clear();
    s.extend(characters);
}}
"
        )?;
    }
    if has_byte_array {
        writeln!(
            r,
            "

pub enum QByteArray {{}}"
        )?;
    }
    if has_list_or_tree {
        writeln!(
            r,
            "

#[repr(C)]
#[derive(PartialEq, Eq, Debug)]
pub enum SortOrder {{
    Ascending = 0,
    Descending = 1,
}}

#[repr(C)]
pub struct QModelIndex {{
    row: c_int,
    internal_id: usize,
}}"
        )?;
    }

    if has_string || has_byte_array || has_list_or_tree {
        writeln!(
            r,
            "

fn to_usize(n: c_int) -> usize {{
    if n < 0 {{
        panic!(\"Cannot cast {{}} to usize\", n);
    }}
    n as usize
}}
"
        )?;
    }

    if has_string || has_byte_array || has_list_or_tree {
        writeln!(
            r,
            "
fn to_c_int(n: usize) -> c_int {{
    if n > c_int::max_value() as usize {{
        panic!(\"Cannot cast {{}} to c_int\", n);
    }}
    n as c_int
}}
"
        )?;
    }
    Ok(())
}

pub fn write_interface(conf: &Config) -> Result<()> {
    let mut r = Vec::new();
    writeln!(
        r,
        "/* generated by rust_qt_binding_generator */
use libc::{{c_char, c_ushort, c_int}};
use std::slice;
use std::char::decode_utf16;

use std::sync::Arc;
use std::sync::atomic::{{AtomicPtr, Ordering}};
use std::ptr::null;

use {}{}::*;",
        get_module_prefix(conf),
        conf.rust.implementation_module
    )?;

    write_rust_types(conf, &mut r)?;

    for object in conf.objects.values() {
        write_rust_interface_object(&mut r, object, conf)?;
    }
    let mut file = conf
        .config_file
        .parent()
        .unwrap()
        .join(&conf.rust.dir)
        .join("src")
        .join(&conf.rust.interface_module);
    file.set_extension("rs");
    write_if_different(file, &r)
}

fn write_rust_implementation_object(r: &mut Vec<u8>, o: &Object) -> Result<()> {
    if o.object_type != ObjectType::Object {
        writeln!(r, "#[derive(Default, Clone)]")?;
        writeln!(r, "struct {}Item {{", o.name)?;
        for (name, ip) in &o.item_properties {
            let lc = snake_case(name);
            if ip.optional {
                writeln!(
                    r,
                    "    {}: Option<{}>,",
                    lc,
                    ip.item_property_type.rust_type()
                )?;
            } else {
                writeln!(r, "    {}: {},", lc, ip.item_property_type.rust_type())?;
            }
        }
        writeln!(r, "}}\n")?;
    }
    let mut model_struct = String::new();
    writeln!(r, "pub struct {} {{\n    emit: {0}Emitter,", o.name)?;
    if o.object_type == ObjectType::List {
        model_struct = format!(", model: {}List", o.name);
        writeln!(r, "    model: {}List,", o.name)?;
    } else if o.object_type == ObjectType::Tree {
        model_struct = format!(", model: {}Tree", o.name);
        writeln!(r, "    model: {}Tree,", o.name)?;
    }
    for (name, p) in &o.properties {
        let lc = snake_case(name);
        writeln!(r, "    {}: {},", lc, rust_type(p))?;
    }
    if o.object_type != ObjectType::Object {
        writeln!(r, "    list: Vec<{}Item>,", o.name)?;
    }
    writeln!(r, "}}\n")?;
    for (name, p) in &o.properties {
        if p.is_object() {
            model_struct += &format!(", {}: {}", name, p.type_name());
        }
    }
    writeln!(
        r,
        "impl {}Trait for {0} {{
    fn new(emit: {0}Emitter{}) -> {0} {{
        {0} {{
            emit,",
        o.name, model_struct
    )?;
    if o.object_type != ObjectType::Object {
        writeln!(r, "            model,")?;
        writeln!(r, "            list: Vec::new(),")?;
    }
    for (name, p) in &o.properties {
        let lc = snake_case(name);
        if p.is_object() {
            writeln!(r, "            {},", lc)?;
        } else {
            writeln!(r, "            {}: {},", lc, rust_type_init(p))?;
        }
    }
    writeln!(
        r,
        "        }}
    }}
    fn emit(&mut self) -> &mut {}Emitter {{
        &self.emit
    }}",
        o.name
    )?;
    for (name, p) in &o.properties {
        let lc = snake_case(name);
        if p.is_object() {
            writeln!(
                r,
                "    fn {}(&self) -> &{} {{
        &self.{0}
    }}
    fn {0}_mut(&mut self) -> &mut {1} {{
        &mut self.{0}
    }}",
                lc,
                rust_return_type(p)
            )?;
        } else if p.rust_by_function {
            writeln!(
                r,
                "    fn {}<F>(&self, getter: F)
    where
        F: FnOnce({}),
    {{
        getter(&self.{0})
    }}",
                lc,
                rust_return_type(p)
            )?;
        } else {
            writeln!(r, "    fn {}(&self) -> {} {{", lc, rust_return_type(p))?;
            if p.is_complex() {
                if p.optional {
                    writeln!(r, "        self.{}.as_ref().map(|p| &p[..])", lc)?;
                } else {
                    writeln!(r, "        &self.{}", lc)?;
                }
            } else {
                writeln!(r, "        self.{}", lc)?;
            }
            writeln!(r, "    }}")?;
        }
        if !p.is_object() && p.write {
            let bytearray = p.property_type == Type::Simple(SimpleType::QByteArray);
            let (t, v) = if bytearray && p.optional {
                ("Option<&[u8]>".to_string(), ".map(|v| v.to_vec())")
            } else if bytearray {
                ("&[u8]".to_string(), ".to_vec()")
            } else {
                (rust_type(p), "")
            };
            writeln!(
                r,
                "    fn set_{}(&mut self, value: {}) {{
        self.{0} = value{};
        self.emit.{0}_changed();
    }}",
                lc, t, v
            )?;
        }
    }
    if o.object_type == ObjectType::List {
        writeln!(
            r,
            "    fn row_count(&self) -> usize {{\n        self.list.len()\n    }}"
        )?;
    } else if o.object_type == ObjectType::Tree {
        writeln!(
            r,
            "    fn row_count(&self, item: Option<usize>) -> usize {{
        self.list.len()
    }}
    fn index(&self, item: Option<usize>, row: usize) -> usize {{
        0
    }}
    fn parent(&self, index: usize) -> Option<usize> {{
        None
    }}
    fn row(&self, index: usize) -> usize {{
        index
    }}
    fn check_row(&self, index: usize, _row: usize) -> Option<usize> {{
        if index < self.list.len() {{
            Some(index)
        }} else {{
            None
        }}
    }}"
        )?;
    }
    if o.object_type != ObjectType::Object {
        for (name, ip) in &o.item_properties {
            let lc = snake_case(name);
            writeln!(
                r,
                "    fn {}(&self, index: usize) -> {} {{",
                lc,
                rust_return_type_(ip)
            )?;
            if ip.is_complex() && ip.optional {
                writeln!(
                    r,
                    "        self.list[index].{}.as_ref().map(|v| &v[..])",
                    lc
                )?;
            } else if ip.is_complex() {
                writeln!(r, "        &self.list[index].{}", lc)?;
            } else {
                writeln!(r, "        self.list[index].{}", lc)?;
            }
            writeln!(r, "    }}")?;
            let bytearray = ip.item_property_type == SimpleType::QByteArray;
            if ip.write && bytearray && ip.optional {
                writeln!(
                    r,
                    "    fn set_{}(&mut self, index: usize, v: Option<&[u8]>) -> bool {{
        self.list[index].{0} = v.map(|v| v.to_vec());
        true
    }}",
                    lc
                )?;
            } else if ip.write && bytearray {
                writeln!(
                    r,
                    "    fn set_{}(&mut self, index: usize, v: &[u8]) -> bool {{
        self.list[index].{0} = v.to_vec();
        true
    }}",
                    lc
                )?;
            } else if ip.write {
                writeln!(
                    r,
                    "    fn set_{}(&mut self, index: usize, v: {}) -> bool {{
        self.list[index].{0} = v;
        true
    }}",
                    lc,
                    rust_type_(ip)
                )?;
            }
        }
    }
    writeln!(r, "}}")
}

pub fn write_implementation(conf: &Config) -> Result<()> {
    let mut file = conf
        .config_file
        .parent()
        .unwrap()
        .join(&conf.rust.dir)
        .join("src")
        .join(&conf.rust.implementation_module);
    if !conf.overwrite_implementation && file.exists() {
        return Ok(());
    }
    file.set_extension("rs");
    if !conf.overwrite_implementation && file.exists() {
        return Ok(());
    }
    let mut r = Vec::new();
    writeln!(
        r,
        "#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use {}{}::*;
",
        get_module_prefix(conf),
        conf.rust.interface_module
    )?;

    for object in conf.objects.values() {
        write_rust_implementation_object(&mut r, object)?;
    }
    write_if_different(file, &r)
}

/// Inspects the rust edition of the target crate to decide how the module
/// imports should be written.
///
/// As of Rust 2018, modules inside the crate should be prefixed with `crate::`.
/// Prior to the 2018 edition, crate-local modules could be imported without
/// this prefix.
fn get_module_prefix(conf: &Config) -> &'static str {
    match conf.rust_edition {
        RustEdition::Rust2018 => "crate::",
        _ => "",
    }
}
