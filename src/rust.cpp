#include "structs.h"
#include "helper.h"

template <typename T>
QString rustType(const T& p)
{
    if (p.optional) {
        return "Option<" + p.type.rustType + ">";
    }
    return p.type.rustType;
}

template <typename T>
QString rustCType(const T& p)
{
    if (p.optional) {
        return "COption<" + p.type.rustType + ">";
    }
    return p.type.rustType;
}

template <typename T>
QString rustTypeInit(const T& p)
{
    if (p.optional) {
        return "None";
    }
    return p.type.rustTypeInit;
}

void writeRustInterfaceObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    r << QString(R"(
pub struct %1QObject {}

#[derive (Clone)]
pub struct %1Emitter {
    qobject: Arc<Mutex<*const %1QObject>>,
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString("    %2_changed: fn(*const %1QObject),\n")
            .arg(o.name, snakeCase(p.name));
    }
    if (o.type == ObjectType::List) {
        r << QString("    new_data_ready: fn(*const %1QObject),\n")
            .arg(o.name);
    } else if (o.type == ObjectType::UniformTree) {
        r << QString("    new_data_ready: fn(*const %1QObject, item: usize, valid: bool),\n")
            .arg(o.name);
    }
    r << QString(R"(}

unsafe impl Send for %1Emitter {}

impl %1Emitter {
    fn clear(&self) {
        *self.qobject.lock().unwrap() = null();
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString(R"(    pub fn %1_changed(&self) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
            (self.%1_changed)(ptr);
        }
    }
)").arg(snakeCase(p.name));
    }
    if (o.type == ObjectType::List) {
        r << R"(    pub fn new_data_ready(&self) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
            (self.new_data_ready)(ptr);
        }
    }
)";
    } else if (o.type == ObjectType::UniformTree) {
        r << R"(    pub fn new_data_ready(&self, item: Option<usize>) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
             (self.new_data_ready)(ptr, item.unwrap_or(13), item.is_some());
        }
    }
)";
    }

    QString modelStruct = "";
    if (o.type != ObjectType::Object) {
        QString type = o.type == ObjectType::List ? "List" : "UniformTree";
        modelStruct = ", model: " + o.name + type;
        QString index;
        QString indexDecl;
        QString indexCDecl;
        if (o.type == ObjectType::UniformTree) {
            indexDecl = " item: Option<usize>,";
            indexCDecl = " item: usize, valid: bool,";
            index = " item.unwrap_or(13), item.is_some(),";
        }
        r << QString(R"(}

pub struct %1%2 {
    qobject: *const %1QObject,
    begin_reset_model: fn(*const %1QObject),
    end_reset_model: fn(*const %1QObject),
    begin_insert_rows: fn(*const %1QObject,%5 usize, usize),
    end_insert_rows: fn(*const %1QObject),
    begin_remove_rows: fn(*const %1QObject,%5 usize, usize),
    end_remove_rows: fn(*const %1QObject),
}

impl %1%2 {
    pub fn begin_reset_model(&self) {
        (self.begin_reset_model)(self.qobject);
    }
    pub fn end_reset_model(&self) {
        (self.end_reset_model)(self.qobject);
    }
    pub fn begin_insert_rows(&self,%3 first: usize, last: usize) {
        (self.begin_insert_rows)(self.qobject,%4 first, last);
    }
    pub fn end_insert_rows(&self) {
        (self.end_insert_rows)(self.qobject);
    }
    pub fn begin_remove_rows(&self,%3 first: usize, last: usize) {
        (self.begin_remove_rows)(self.qobject,%4 first, last);
    }
    pub fn end_remove_rows(&self) {
        (self.end_remove_rows)(self.qobject);
    }
)").arg(o.name, type, indexDecl, index, indexCDecl);
    }

    r << QString(R"(}

pub trait %1Trait {
    fn create(emit: %1Emitter%2) -> Self;
    fn emit(&self) -> &%1Emitter;
)").arg(o.name, modelStruct);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2;\n").arg(lc, rustType(p));
        if (p.write) {
            r << QString("    fn set_%1(&mut self, value: %2);\n").arg(lc, rustType(p));
        }
    }
    if (o.type == ObjectType::List) {
        r << R"(    fn row_count(&self) -> usize;
    fn can_fetch_more(&self) -> bool { false }
    fn fetch_more(&mut self) {}
    fn sort(&mut self, u8, SortOrder) {}
)";
    } else if (o.type == ObjectType::UniformTree) {
        r << R"(    fn row_count(&self, Option<usize>) -> usize;
    fn can_fetch_more(&self, Option<usize>) -> bool { false }
    fn fetch_more(&mut self, Option<usize>) {}
    fn sort(&mut self, u8, SortOrder) {}
    fn index(&self, item: Option<usize>, row: usize) -> usize;
    fn parent(&self, item: usize) -> Option<usize>;
    fn row(&self, item: usize) -> usize;
)";
    }
    if (o.type != ObjectType::Object) {
        for (auto ip: o.itemProperties) {
            r << QString("    fn %1(&self, item: usize) -> %2;\n")
                    .arg(snakeCase(ip.name), rustType(ip));
            if (ip.write) {
                r << QString("    fn set_%1(&mut self, item: usize, %2) -> bool;\n")
                    .arg(snakeCase(ip.name), rustType(ip));
            }
        }
    }

    r << QString(R"(}

#[no_mangle]
pub extern "C" fn %2_new(qobject: *const %1QObject)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        r << QString(",\n        %2_changed: fn(*const %1QObject)")
            .arg(o.name, snakeCase(p.name));
    }
    if (o.type == ObjectType::List) {
        r << QString(",\n        new_data_ready: fn(*const %1QObject)")
            .arg(o.name);
    } else if (o.type == ObjectType::UniformTree) {
        r << QString(",\n        new_data_ready: fn(*const %1QObject, item: usize, valid: bool)")
            .arg(o.name);
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl;
        if (o.type == ObjectType::UniformTree) {
            indexDecl = " item: usize, valid: bool,";
        }
        r << QString(R"(,
        begin_reset_model: fn(*const %1QObject),
        end_reset_model: fn(*const %1QObject),
        begin_insert_rows: fn(*const %1QObject,%2
            usize,
            usize),
        end_insert_rows: fn(*const %1QObject),
        begin_remove_rows: fn(*const %1QObject,%2
            usize,
            usize),
        end_remove_rows: fn(*const %1QObject))").arg(o.name, indexDecl);
    }
    r << QString(R"()
        -> *mut %1 {
    let emit = %1Emitter {
        qobject: Arc::new(Mutex::new(qobject)),
)").arg(o.name);
    for (const Property& p: o.properties) {
        r << QString("        %1_changed: %1_changed,\n").arg(snakeCase(p.name));
    }
    if (o.type != ObjectType::Object) {
        r << QString("        new_data_ready: new_data_ready,\n");
    }
    QString model = "";
    if (o.type != ObjectType::Object) {
        const QString type = o.type == ObjectType::List ? "List" : "UniformTree";
        model = ", model";
        r << QString(R"(    };
    let model = %1%2 {
        qobject: qobject,
        begin_reset_model: begin_reset_model,
        end_reset_model: end_reset_model,
        begin_insert_rows: begin_insert_rows,
        end_insert_rows: end_insert_rows,
        begin_remove_rows: begin_remove_rows,
        end_remove_rows: end_remove_rows,
)").arg(o.name, type);
    }
    r << QString(R"(    };
    let d = %1::create(emit%3);
    Box::into_raw(Box::new(d))
}

#[no_mangle]
pub unsafe extern "C" fn %2_free(ptr: *mut %1) {
    Box::from_raw(ptr).emit().clear();
}
)").arg(o.name, lcname, model);
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        QString ret = ") -> " + rustType(p);
        if (p.type.isComplex() && !p.optional) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1,
        p: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).get_%3();
    set(p, %4::from(&data));
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            if (p.write) {
                const QString type = p.type.name == "QString" ? "QStringIn" : p.type.name;
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v.convert());
}
)").arg(o.name, base, snakeCase(p.name), type);
            }
        } else if (p.type.isComplex()) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1,
        p: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).get_%3();
    if let Some(data) = data {
        set(p, %4::from(&data));
    }
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            if (p.write) {
                const QString type = p.type.name == "QString" ? "QStringIn" : p.type.name;
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(Some(v.convert()));
}
#[no_mangle]
pub unsafe extern "C" fn %2_set_none(ptr: *mut %1) {
    (&mut *ptr).set_%3(None);
}
)").arg(o.name, base, snakeCase(p.name), type);
            }
        } else {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1) -> %4 {
    (&*ptr).get_%3()
}
)").arg(o.name, base, snakeCase(p.name), rustType(p));
            if (p.write) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(v);
}
)").arg(o.name, base, snakeCase(p.name), rustType(p));
            }
        }
    }
    if (o.type == ObjectType::List) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(ptr: *const %1) -> c_int {
    (&*ptr).row_count() as c_int
}
#[no_mangle]
pub unsafe extern "C" fn %2_can_fetch_more(ptr: *const %1) -> bool {
    (&*ptr).can_fetch_more()
}
#[no_mangle]
pub unsafe extern "C" fn %2_fetch_more(ptr: *mut %1) {
    (&mut *ptr).fetch_more()
}
#[no_mangle]
pub unsafe extern "C" fn %2_sort(ptr: *mut %1, column: u8, order: SortOrder) {
    (&mut *ptr).sort(column, order)
}
)").arg(o.name, lcname);
    } else if (o.type == ObjectType::UniformTree) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(ptr: *const %1, item: usize, valid: bool) -> c_int {
    if valid {
        (&*ptr).row_count(Some(item)) as c_int
    } else {
        (&*ptr).row_count(None) as c_int
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_can_fetch_more(ptr: *const %1, item: usize, valid: bool) -> bool {
    if valid {
        (&*ptr).can_fetch_more(Some(item))
    } else {
        (&*ptr).can_fetch_more(None)
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_fetch_more(ptr: *mut %1, item: usize, valid: bool) {
    if valid {
        (&mut *ptr).fetch_more(Some(item))
    } else {
        (&mut *ptr).fetch_more(None)
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_sort(ptr: *mut %1, column: u8, order: SortOrder) {
    (&mut *ptr).sort(column, order)
}
#[no_mangle]
pub unsafe extern "C" fn %2_index(ptr: *const %1, item: usize, valid: bool, row: c_int) -> usize {
    if !valid {
        (&*ptr).index(None, row as usize)
    } else {
        (&*ptr).index(Some(item), row as usize)
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_parent(ptr: *const %1, index: usize) -> QModelIndex {
    if let Some(parent) = (&*ptr).parent(index) {
        QModelIndex{row: (&*ptr).row(parent) as c_int, internal_id: parent}
    } else {
        QModelIndex{row: -1, internal_id: 0}
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_row(ptr: *const %1, item: usize) -> c_int {
    (&*ptr).row(item) as c_int
}
)").arg(o.name, lcname);
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl = ", row: c_int";
        QString index = "row as usize";
        if (o.type == ObjectType::UniformTree) {
            indexDecl = ", item: usize";
            index = "item";
        }
        for (auto ip: o.itemProperties) {
            if (ip.type.isComplex() && !ip.optional) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1%5,
        d: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).%3(%6);
    set(d, %4::from(&data));
}
)").arg(o.name, lcname, snakeCase(ip.name), ip.type.name, indexDecl, index);
            } else if (ip.type.isComplex()) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1%5,
        d: *mut c_void,
        set: fn(*mut c_void, %4)) {
    let data = (&*ptr).%3(%6);
    if let Some(data) = data {
        set(d, %4::from(&data));
    }
}
)").arg(o.name, lcname, snakeCase(ip.name), ip.type.name, indexDecl, index);
            } else {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_data_%3(ptr: *const %1%5) -> %4 {
    (&*ptr).%3(%6).into()
}
)").arg(o.name, lcname, snakeCase(ip.name), rustCType(ip), indexDecl, index);
            }
            if (ip.write) {
                QString val = "v";
                QString type = ip.type.rustType;
                if (ip.type.isComplex()) {
                    val = val + ".convert()";
                    type = ip.type.name == "QString" ? "QStringIn" : ip.type.name;
                }
                if (ip.optional) {
                    val = "Some(" + val + ")";
                }
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set_data_%3(ptr: *mut %1%4, v: %6) -> bool {
    (&mut *ptr).set_%3(%5, %7)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, type, val);
            }
            if (ip.write && ip.optional) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set_data_%3_none(ptr: *mut %1, row: c_int%4) -> bool {
    (&mut *ptr).set_%3(%5row as usize, None)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index);
            }
        }
    }
}

QString rustFile(const QDir rustdir, const QString& module) {
    QDir src(rustdir.absoluteFilePath("src"));
    QString path = src.absoluteFilePath(module + ".rs");
    return path;
}

void writeRustTypes(const Configuration& conf, QTextStream& r) {
    bool hasOption = false;
    bool hasString = false;
    bool hasStringWrite = false;
    bool hasByteArray = false;
    bool hasListOrTree = false;

    for (auto o: conf.objects) {
        hasListOrTree |= o.type != ObjectType::Object;
        for (auto p: o.properties) {
            hasOption |= p.optional;
            hasString |= p.type.type == BindingType::QString;
            hasStringWrite |= p.type.type == BindingType::QString
                    && p.write;
            hasByteArray |= p.type.type == BindingType::QByteArray;
        }
        for (auto p: o.itemProperties) {
            hasOption |= p.optional;
            hasString |= p.type.type == BindingType::QString;
            hasByteArray |= p.type.type == BindingType::QByteArray;
        }
    }

    if (hasOption || hasListOrTree) {
        r << R"(

#[repr(C)]
pub struct COption<T> {
    data: T,
    some: bool,
}

impl<T> From<Option<T>> for COption<T> where T: Default {
    fn from(t: Option<T>) -> COption <T> {
        if let Some(v) = t {
            COption {
                data: v,
                some: true
            }
        } else {
            COption {
                data: T::default(),
                some: false
            }
        }
    }
}
)";
    }
    if (hasString) {
        r << R"(

#[repr(C)]
pub struct QString {
    data: *const uint8_t,
    len: c_int,
}

#[repr(C)]
pub struct QStringIn {
    data: *const uint16_t,
    len: c_int,
}

impl QStringIn {
    fn convert(&self) -> String {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        String::from_utf16_lossy(data)
    }
}

impl<'a> From<&'a String> for QString {
    fn from(string: &'a String) -> QString {
        QString {
            len: string.len() as c_int,
            data: string.as_ptr(),
        }
    }
}
)";
                 }
    if (hasByteArray) {
        r << R"(

#[repr(C)]
pub struct QByteArray {
    data: *const uint8_t,
    len: c_int,
}

impl QByteArray {
    fn convert(&self) -> Vec<u8> {
        let data = unsafe { slice::from_raw_parts(self.data, self.len as usize) };
        Vec::from(data)
    }
}

impl<'a> From<&'a Vec<u8>> for QByteArray {
    fn from(value: &'a Vec<u8>) -> QByteArray {
        QByteArray {
            len: value.len() as c_int,
            data: value.as_ptr(),
        }
    }
}
)";
    }
    if (hasListOrTree) {
        r << R"(

#[repr(C)]
pub enum SortOrder {
    Ascending = 0,
    Descending = 1
}

#[repr(C)]
pub struct QModelIndex {
    row: c_int,
    internal_id: usize,
}
)";
    }
}

void writeRustInterface(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.interfaceModule));
    QTextStream r(&w.buffer);
    r << QString(R"(/* generated by rust_qt_binding_generator */
#![allow(unknown_lints)]
#![allow(mutex_atomic, needless_pass_by_value)]
use libc::{c_int, c_void, uint8_t, uint16_t};
use std::slice;

use std::sync::{Arc, Mutex};
use std::ptr::null;

use %1::*;
)").arg(conf.implementationModule);

    writeRustTypes(conf, r);

    for (auto object: conf.objects) {
        writeRustInterfaceObject(r, object);
    }
}

void writeRustImplementationObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    if (o.type != ObjectType::Object) {
        r << "#[derive (Default, Clone)]\n";
        r << QString("struct %1Item {\n").arg(o.name);
        for (auto ip: o.itemProperties) {
            const QString lc(snakeCase(ip.name));
            r << QString("    %1: %2,\n").arg(lc, ip.type.rustType);
        }
        r << "}\n\n";
    }
    QString modelStruct = "";
    r << QString("pub struct %1 {\n    emit: %1Emitter,\n").arg((o.name));
    if (o.type == ObjectType::List) {
        modelStruct = ", model: " + o.name + "List";
        r << QString("    model: %1List,\n").arg(o.name);
    } else if (o.type == ObjectType::UniformTree) {
        modelStruct = ", model: " + o.name + "UniformTree";
        r << QString("    model: %1UniformTree,\n").arg(o.name);
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    %1: %2,\n").arg(lc, rustType(p));
    }
    if (o.type != ObjectType::Object) {
        r << QString("    list: Vec<%1Item>,\n").arg(o.name);
    }
    r << "}\n\n";
    r << QString(R"(impl %1Trait for %1 {
    fn create(emit: %1Emitter%2) -> %1 {
        %1 {
            emit: emit,
)").arg(o.name, modelStruct);
    if (o.type != ObjectType::Object) {
        r << QString("            model: model,\n");
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("            %1: %2,\n").arg(lc, rustTypeInit(p));
    }
    if (o.type != ObjectType::Object) {
        r << QString("            list: vec![%1Item::default(); 10],\n")
            .arg(o.name);
    }
    r << QString(R"(        }
    }
    fn emit(&self) -> &%1Emitter {
        &self.emit
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    fn get_%1(&self) -> %2 {\n").arg(lc, rustType(p));
        if (p.type.isComplex()) {
            r << QString("        self.%1.clone()\n").arg(lc);
        } else {
            r << QString("        self.%1\n").arg(lc);
        }
        r << "    }\n";
        if (p.write) {
            r << QString(R"(    fn set_%1(&mut self, value: %2) {
        self.%1 = value;
        self.emit.%1_changed();
    }
)").arg(lc, rustType(p));
        }
    }
    if (o.type == ObjectType::List) {
        r << "    fn row_count(&self) -> usize {\n        self.list.len()\n    }\n";
    } else if (o.type == ObjectType::UniformTree) {
        r << R"(    fn row_count(&self, item: Option<usize>) -> usize {
        self.list.len()
    }
    fn index(&self, item: Option<usize>, row: usize) -> usize {
        0
    }
    fn parent(&self, item: usize) -> Option<usize> {
        None
    }
    fn row(&self, item: usize) -> usize {
        item
    }
)";
    }
    if (o.type != ObjectType::Object) {
        QString index;
        if (o.type == ObjectType::UniformTree) {
            index = ", item: usize";
        }
        for (auto ip: o.itemProperties) {
            const QString lc(snakeCase(ip.name));
            r << QString("    fn %1(&self, item: usize) -> %2 {\n")
                    .arg(lc, rustType(ip));
            r << "        self.list[item]." << lc << ".clone()\n";
            r << "    }\n";
            if (ip.write) {
                r << QString("    fn set_%1(&mut self, item: usize, v: %2) -> bool {\n")
                        .arg(snakeCase(ip.name), rustType(ip));
                r << "        self.list[item]." << lc << " = v;\n";
                r << "        true\n";
                r << "    }\n";
            }
        }
    }
    r << "}\n";
}

void writeRustImplementation(const Configuration& conf) {
    DifferentFileWriter w(rustFile(conf.rustdir, conf.implementationModule),
        conf.overwriteImplementation);
    QTextStream r(&w.buffer);
    r << QString(R"(#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(dead_code)]
use %1::*;

)").arg(conf.interfaceModule);

    for (auto object: conf.objects) {
        writeRustImplementationObject(r, object);
    }
}
