/*
 *   Copyright 2017  Jos van den Oever <jos@vandenoever.info>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
QString rustReturnType(const T& p)
{
    QString type = p.type.rustType;
    if (type == "String" && !p.rustByValue) {
        type = "str";
    }
    if (type == "Vec<u8>" && !p.rustByValue) {
        type = "[u8]";
    }
    if (p.type.isComplex() && !p.rustByValue) {
        type = "&" + type;
    }
    if (p.optional) {
        return "Option<" + type + ">";
    }
    return type;
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

void rConstructorArgsDecl(QTextStream& r, const QString& name, const Object& o, const Configuration& conf) {
    r << QString("    %2: *mut %1QObject").arg(o.name, snakeCase(name));
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            r << QString(",\n");
            rConstructorArgsDecl(r, p.name, conf.findObject(p.type.name), conf);
        } else {
            r << QString(",\n    %3_%2_changed: fn(*const %1QObject)")
                .arg(o.name, snakeCase(p.name), snakeCase(name));
        }
    }
    if (o.type == ObjectType::List) {
        r << QString(",\n    %2_new_data_ready: fn(*const %1QObject)")
            .arg(o.name, snakeCase(name));
    } else if (o.type == ObjectType::Tree) {
        r << QString(",\n    %2_new_data_ready: fn(*const %1QObject, index: COption<usize>)")
            .arg(o.name, snakeCase(name));
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl;
        if (o.type == ObjectType::Tree) {
            indexDecl = " index: COption<usize>,";
        }
        r << QString(R"(,
    %3_data_changed: fn(*const %1QObject, usize, usize),
    %3_begin_reset_model: fn(*const %1QObject),
    %3_end_reset_model: fn(*const %1QObject),
    %3_begin_insert_rows: fn(*const %1QObject,%2 usize, usize),
    %3_end_insert_rows: fn(*const %1QObject),
    %3_begin_remove_rows: fn(*const %1QObject,%2 usize, usize),
    %3_end_remove_rows: fn(*const %1QObject))").arg(o.name, indexDecl,
          snakeCase(name));
    }
}

void rConstructorArgs(QTextStream& r, const QString& name, const Object& o, const Configuration& conf) {
    const QString lcname(snakeCase(o.name));
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            rConstructorArgs(r, p.name, conf.findObject(p.type.name), conf);
        }
    }
    r << QString(R"(    let %2_emit = %1Emitter {
        qobject: Arc::new(Mutex::new(%2)),
)").arg(o.name, snakeCase(name));
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) continue;
        r << QString("        %1_changed: %2_%1_changed,\n").arg(snakeCase(p.name), snakeCase(name));
    }
    if (o.type != ObjectType::Object) {
        r << QString("        new_data_ready: %1_new_data_ready,\n")
            .arg(snakeCase(name));
    }
    QString model = "";
    if (o.type != ObjectType::Object) {
        const QString type = o.type == ObjectType::List ? "List" : "Tree";
        model = ", model";
        r << QString(R"(    };
    let model = %1%2 {
        qobject: %3,
        data_changed: %4_data_changed,
        begin_reset_model: %4_begin_reset_model,
        end_reset_model: %4_end_reset_model,
        begin_insert_rows: %4_begin_insert_rows,
        end_insert_rows: %4_end_insert_rows,
        begin_remove_rows: %4_begin_remove_rows,
        end_remove_rows: %4_end_remove_rows,
)").arg(o.name, type, snakeCase(name), snakeCase(name));
    }
    r << QString("    };\n    let d_%3 = %1::new(%3_emit%2")
         .arg(o.name, model, snakeCase(name));
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            r << ",\n        d_" << snakeCase(p.name);
        }
    }
    r << ");\n";
}

void writeFunction(QTextStream& r, const Function& f, const QString& lcname, const Object& o) {
    const QString lc(snakeCase(f.name));
    r << QString(R"(
#[no_mangle]
pub extern "C" fn %1_%2(ptr: *%3 %4)").arg(lcname, lc, f.mut ? "mut" : "const", o.name);
    // write all the input arguments, for QString and QByteArray, write
    // pointers to their content and the length which is int in Qt
    for (auto a = f.args.begin(); a < f.args.end(); a++) {
        r << ", ";
        if (a->type.name == "QString") {
            r << QString("%1_str: *const c_ushort, %1_len: c_int").arg(a->name);
        } else if (a->type.name == "QByteArray") {
            r << QString("%1_str: *const c_char, %1_len: c_int").arg(a->name);
        } else {
            r << a->name << ": " << a->type.rustType;
        }
    }
    // If the return type is QString or QByteArray, append a pointer to the
    // variable that will be set to the argument list. Also add a setter
    // function.
    if (f.type.isComplex()) {
        r << QString(", d: *mut %1, set: fn(*mut %1, str: *const c_char, len: c_int)) {\n").arg(f.type.name);
    } else {
        r << ") -> " << f.type.rustType << " {\n";
    }
    for (auto a = f.args.begin(); a < f.args.end(); a++) {
        if (a->type.name == "QString") {
            r << "    let mut " << a->name << " = String::new();\n";
            r << QString("    set_string_from_utf16(&mut %1, %1_str, %1_len);\n").arg(a->name);
        } else if (a->type.name == "QByteArray") {
            r << QString("    let %1 = unsafe { slice::from_raw_parts(%1_str as *const u8, to_usize(%1_len)) };\n").arg(a->name);
        }
    }
    if (f.mut) {
        r << "    let o = unsafe { &mut *ptr };\n";
    } else {
        r << "    let o = unsafe { &*ptr };\n";
    }
    r << "    let r = o." << lc << "(";
    for (auto a = f.args.begin(); a < f.args.end(); a++) {
        if (a != f.args.begin()) {
            r << ", ";
        }
        r << a->name;
    }
    r << ");\n";
    if (f.type.isComplex()) {
        r << "    let s: *const c_char = r.as_ptr() as (*const c_char);\n";
        r << "    set(d, s, r.len() as i32);\n";
    } else {
        r << "    r\n";
    }
    r << "}\n";
}

void writeRustInterfaceObject(QTextStream& r, const Object& o, const Configuration& conf) {
    const QString lcname(snakeCase(o.name));
    r << QString(R"(
pub struct %1QObject {}

#[derive(Clone)]
pub struct %1Emitter {
    qobject: Arc<Mutex<*const %1QObject>>,
)").arg(o.name);
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            continue;
        }
        r << QString("    %2_changed: fn(*const %1QObject),\n")
            .arg(o.name, snakeCase(p.name));
    }
    if (o.type == ObjectType::List) {
        r << QString("    new_data_ready: fn(*const %1QObject),\n")
            .arg(o.name);
    } else if (o.type == ObjectType::Tree) {
        r << QString("    new_data_ready: fn(*const %1QObject, index: COption<usize>),\n")
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
        if (p.type.type == BindingType::Object) {
            continue;
        }
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
    } else if (o.type == ObjectType::Tree) {
        r << R"(    pub fn new_data_ready(&self, item: Option<usize>) {
        let ptr = *self.qobject.lock().unwrap();
        if !ptr.is_null() {
            (self.new_data_ready)(ptr, item.into());
        }
    }
)";
    }

    QString modelStruct = "";
    if (o.type != ObjectType::Object) {
        QString type = o.type == ObjectType::List ? "List" : "Tree";
        modelStruct = ", model: " + o.name + type;
        QString index;
        QString indexDecl;
        QString indexCDecl;
        if (o.type == ObjectType::Tree) {
            indexDecl = " index: Option<usize>,";
            indexCDecl = " index: COption<usize>,";
            index = " index.into(),";
        }
        r << QString(R"(}

pub struct %1%2 {
    qobject: *const %1QObject,
    data_changed: fn(*const %1QObject, usize, usize),
    begin_reset_model: fn(*const %1QObject),
    end_reset_model: fn(*const %1QObject),
    begin_insert_rows: fn(*const %1QObject,%5 usize, usize),
    end_insert_rows: fn(*const %1QObject),
    begin_remove_rows: fn(*const %1QObject,%5 usize, usize),
    end_remove_rows: fn(*const %1QObject),
}

impl %1%2 {
    pub fn data_changed(&self, first: usize, last: usize) {
        (self.data_changed)(self.qobject, first, last);
    }
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
    fn new(emit: %1Emitter%2)").arg(o.name, modelStruct);
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            r << ",\n        " << snakeCase(p.name) << ": " << p.type.name;
        }
    }
    r << QString(R"() -> Self;
    fn emit(&self) -> &%1Emitter;
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        if (p.type.type == BindingType::Object) {
            r << QString("    fn %1(&self) -> &%2;\n").arg(lc, rustType(p));
            r << QString("    fn %1_mut(&mut self) -> &mut %2;\n").arg(lc, rustType(p));
        } else {
            if (p.rustByFunction) {
                r << QString("    fn %1<F>(&self, getter: F) where F: FnOnce(%2);").arg(lc, rustReturnType(p));
            } else {
                r << QString("    fn %1(&self) -> %2;\n").arg(lc, rustReturnType(p));
            }
            if (p.write) {
                if (p.type.name == "QByteArray") {
                    if (p.optional) {
                        r << QString("    fn set_%1(&mut self, value: Option<&[u8]>);\n").arg(lc);
                    } else {
                        r << QString("    fn set_%1(&mut self, value: &[u8]);\n").arg(lc);
                    }
                } else {
                    r << QString("    fn set_%1(&mut self, value: %2);\n").arg(lc, rustType(p));
                }
            }
        }
    }
    for (const Function& f: o.functions) {
        const QString lc(snakeCase(f.name));
        QString argList;
        if (f.args.size() > 0) {
            for (auto a = f.args.begin(); a < f.args.end(); a++) {
                auto t = a->type.name == "QByteArray" ?"&[u8]" :a->type.rustType;
                argList.append(QString(", %1: %2").arg(a->name, t));
            }
        }
        r << QString("    fn %1(&%2self%4) -> %3;\n")
            .arg(lc, f.mut ? "mut " : "", f.type.rustType, argList);
    }
    if (o.type == ObjectType::List) {
        r << R"(    fn row_count(&self) -> usize;
    fn insert_rows(&mut self, _row: usize, _count: usize) -> bool { false }
    fn remove_rows(&mut self, _row: usize, _count: usize) -> bool { false }
    fn can_fetch_more(&self) -> bool {
        false
    }
    fn fetch_more(&mut self) {}
    fn sort(&mut self, u8, SortOrder) {}
)";
    } else if (o.type == ObjectType::Tree) {
        r << R"(    fn row_count(&self, Option<usize>) -> usize;
    fn can_fetch_more(&self, Option<usize>) -> bool {
        false
    }
    fn fetch_more(&mut self, Option<usize>) {}
    fn sort(&mut self, u8, SortOrder) {}
    fn index(&self, item: Option<usize>, row: usize) -> usize;
    fn parent(&self, index: usize) -> Option<usize>;
    fn row(&self, index: usize) -> usize;
)";
    }
    if (o.type != ObjectType::Object) {
        for (auto ip: o.itemProperties) {
            r << QString("    fn %1(&self, index: usize) -> %2;\n")
                    .arg(snakeCase(ip.name), rustReturnType(ip));
            if (ip.write) {
                if (ip.type.name == "QByteArray") {
                    if (ip.optional) {
                        r << QString("    fn set_%1(&mut self, index: usize, Option<&[u8]>) -> bool;\n")
                            .arg(snakeCase(ip.name));
                    } else {
                        r << QString("    fn set_%1(&mut self, index: usize, &[u8]) -> bool;\n")
                            .arg(snakeCase(ip.name));
                    }
                } else {
                    r << QString("    fn set_%1(&mut self, index: usize, %2) -> bool;\n")
                        .arg(snakeCase(ip.name), rustType(ip));
                }
            }
        }
    }

    r << QString(R"(}

#[no_mangle]
pub extern "C" fn %1_new(
)").arg(lcname);
    rConstructorArgsDecl(r, lcname, o, conf);
    r << QString(",\n) -> *mut %1 {\n").arg(o.name);
    rConstructorArgs(r, lcname, o, conf);
    r << QString(R"(    Box::into_raw(Box::new(d_%2))
}

#[no_mangle]
pub unsafe extern "C" fn %2_free(ptr: *mut %1) {
    Box::from_raw(ptr).emit().clear();
}
)").arg(o.name, lcname);
    for (const Property& p: o.properties) {
        const QString base = QString("%1_%2").arg(lcname, snakeCase(p.name));
        QString ret = ") -> " + rustType(p);
        if (p.type.type == BindingType::Object) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *mut %1) -> *mut %4 {
    (&mut *ptr).%3_mut()
}
)").arg(o.name, base, snakeCase(p.name), rustType(p));

        } else if (p.type.isComplex() && !p.optional) {
            if (p.rustByFunction) {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_get(
    ptr: *const %1,
    p: *mut %4,
    set: fn(*mut %4, *const c_char, c_int),
) {
    let o = unsafe { &*ptr };
    o.%3(|v| {
        let s: *const c_char = v.as_ptr() as (*const c_char);
        set(p, s, to_c_int(v.len()));
    });
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            } else {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_get(
    ptr: *const %1,
    p: *mut %4,
    set: fn(*mut %4, *const c_char, c_int),
) {
    let o = unsafe { &*ptr };
    let v = o.%3();
    let s: *const c_char = v.as_ptr() as (*const c_char);
    set(p, s, to_c_int(v.len()));
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            }
            if (p.write && p.type.name == "QString") {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set(ptr: *mut %1, v: *const c_ushort, len: c_int) {
    let o = unsafe { &mut *ptr };
    let mut s = String::new();
    set_string_from_utf16(&mut s, v, len);
    o.set_%3(s);
}
)").arg(o.name, base, snakeCase(p.name));
            } else if (p.write) {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set(ptr: *mut %1, v: *const c_char, len: c_int) {
    let o = unsafe { &mut *ptr };
    let v = unsafe { slice::from_raw_parts(v as *const u8, to_usize(len)) };
    o.set_%3(v);
}
)").arg(o.name, base, snakeCase(p.name));
            }
        } else if (p.type.isComplex()) {
            r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_get(
    ptr: *const %1,
    p: *mut %4,
    set: fn(*mut %4, *const c_char, c_int),
) {
    let o = unsafe { &*ptr };
    let v = o.%3();
    if let Some(v) = v {
        let s: *const c_char = v.as_ptr() as (*const c_char);
        set(p, s, to_c_int(v.len()));
    }
}
)").arg(o.name, base, snakeCase(p.name), p.type.name);
            if (p.write && p.type.name == "QString") {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set(ptr: *mut %1, v: *const c_ushort, len: c_int) {
    let o = unsafe { &mut *ptr };
    let mut s = String::new();
    set_string_from_utf16(&mut s, v, len);
    o.set_%3(Some(s));
}
)").arg(o.name, base, snakeCase(p.name));
            } else if (p.write) {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set(ptr: *mut %1, v: *const c_char, len: c_int) {
    let o = unsafe { &mut *ptr };
    let v = unsafe { slice::from_raw_parts(v as *const u8, to_usize(len)) };
    o.set_%3(Some(v.into()));
}
)").arg(o.name, base, snakeCase(p.name));
            }
        } else if (p.optional) {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1) -> COption<%4> {
    match (&*ptr).%3() {
        Some(value) => COption { data: value, some: true },
        None => COption { data: %4::default(), some: false}
    }
}
)").arg(o.name, base, snakeCase(p.name), p.type.rustType);
            if (p.write) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set(ptr: *mut %1, v: %4) {
    (&mut *ptr).set_%3(Some(v));
}
)").arg(o.name, base, snakeCase(p.name), p.type.rustType);
            }
        } else {
            r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_get(ptr: *const %1) -> %4 {
    (&*ptr).%3()
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
        if (p.write && p.optional) {
            r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set_none(ptr: *mut %1) {
    let o = unsafe { &mut *ptr };
    o.set_%3(None);
}
)").arg(o.name, base, snakeCase(p.name));
        }
    }
    for (const Function& f: o.functions) {
        writeFunction(r, f, lcname, o);
    }
    if (o.type == ObjectType::List) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(ptr: *const %1) -> c_int {
    to_c_int((&*ptr).row_count())
}
#[no_mangle]
pub unsafe extern "C" fn %2_insert_rows(ptr: *mut %1, row: c_int, count: c_int) -> bool {
    (&mut *ptr).insert_rows(to_usize(row), to_usize(count))
}
#[no_mangle]
pub unsafe extern "C" fn %2_remove_rows(ptr: *mut %1, row: c_int, count: c_int) -> bool {
    (&mut *ptr).remove_rows(to_usize(row), to_usize(count))
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
pub unsafe extern "C" fn %2_sort(
    ptr: *mut %1,
    column: u8,
    order: SortOrder,
) {
    (&mut *ptr).sort(column, order)
}
)").arg(o.name, lcname);
    } else if (o.type == ObjectType::Tree) {
        r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_row_count(
    ptr: *const %1,
    index: COption<usize>,
) -> c_int {
    to_c_int((&*ptr).row_count(index.into()))
}
#[no_mangle]
pub unsafe extern "C" fn %2_can_fetch_more(
    ptr: *const %1,
    index: COption<usize>,
) -> bool {
    (&*ptr).can_fetch_more(index.into())
}
#[no_mangle]
pub unsafe extern "C" fn %2_fetch_more(ptr: *mut %1, index: COption<usize>) {
    (&mut *ptr).fetch_more(index.into())
}
#[no_mangle]
pub unsafe extern "C" fn %2_sort(
    ptr: *mut %1,
    column: u8,
    order: SortOrder
) {
    (&mut *ptr).sort(column, order)
}
#[no_mangle]
pub unsafe extern "C" fn %2_index(
    ptr: *const %1,
    index: COption<usize>,
    row: c_int,
) -> usize {
    (&*ptr).index(index.into(), to_usize(row))
}
#[no_mangle]
pub unsafe extern "C" fn %2_parent(ptr: *const %1, index: usize) -> QModelIndex {
    if let Some(parent) = (&*ptr).parent(index) {
        QModelIndex {
            row: to_c_int((&*ptr).row(parent)),
            internal_id: parent,
        }
    } else {
        QModelIndex {
            row: -1,
            internal_id: 0,
        }
    }
}
#[no_mangle]
pub unsafe extern "C" fn %2_row(ptr: *const %1, index: usize) -> c_int {
    to_c_int((&*ptr).row(index))
}
)").arg(o.name, lcname);
    }
    if (o.type != ObjectType::Object) {
        QString indexDecl = ", row: c_int";
        QString index = "to_usize(row)";
        if (o.type == ObjectType::Tree) {
            indexDecl = ", index: usize";
            index = "index";
        }
        for (auto ip: o.itemProperties) {
            if (ip.type.isComplex() && !ip.optional) {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_data_%3(
    ptr: *const %1%4,
    d: *mut %6,
    set: fn(*mut %6, *const c_char, len: c_int),
) {
    let o = unsafe { &*ptr };
    let data = o.%3(%5);
    let s: *const c_char = data.as_ptr() as (*const c_char);
    set(d, s, to_c_int(data.len()));
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, ip.type.name);
            } else if (ip.type.isComplex()) {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_data_%3(
    ptr: *const %1%4,
    d: *mut %6,
    set: fn(*mut %6, *const c_char, len: c_int),
) {
    let o = unsafe { &*ptr };
    let data = o.%3(%5);
    if let Some(data) = data {
        let s: *const c_char = data.as_ptr() as (*const c_char);
        set(d, s, to_c_int(data.len()));
    }
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, ip.type.name);
            } else {
                r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_data_%3(ptr: *const %1%5) -> %4 {
    let o = unsafe { &*ptr };
    o.%3(%6).into()
}
)").arg(o.name, lcname, snakeCase(ip.name), rustCType(ip), indexDecl, index);
            }
            if (ip.write) {
                QString val = "v";
                if (ip.optional) {
                    val = "Some(" + val + ")";
                }
                if (ip.type.name == "QString") {
                    r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set_data_%3(
    ptr: *mut %1%4,
    s: *const c_ushort, len: c_int,
) -> bool {
    let o = unsafe { &mut *ptr };
    let mut v = String::new();
    set_string_from_utf16(&mut v, s, len);
    o.set_%3(%5, %6)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, val);
                } else if (ip.type.name == "QByteArray") {
                    r << QString(R"(
#[no_mangle]
pub extern "C" fn %2_set_data_%3(
    ptr: *mut %1%4,
    s: *const c_char, len: c_int,
) -> bool {
    let o = unsafe { &mut *ptr };
    let slice = unsafe { ::std::slice::from_raw_parts(s as *const u8, to_usize(len)) };
    o.set_%3(%5, %6)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, ip.optional ?"Some(slice)" :"slice");
                } else {
                    const QString type = ip.type.rustType;
                    r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set_data_%3(
    ptr: *mut %1%4,
    v: %6,
) -> bool {
    (&mut *ptr).set_%3(%5, %7)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index, type, val);
                }
            }
            if (ip.write && ip.optional) {
                r << QString(R"(
#[no_mangle]
pub unsafe extern "C" fn %2_set_data_%3_none(ptr: *mut %1%4) -> bool {
    (&mut *ptr).set_%3(%5, None)
}
)").arg(o.name, lcname, snakeCase(ip.name), indexDecl, index);
            }
        }
    }
}

QString rustFile(const QDir rustdir, const QString& module) {
    QDir src(rustdir.absoluteFilePath("src"));
    QString modulePath = src.absoluteFilePath(module + "/mod.rs");
    if (QFile::exists(modulePath)) {
        return modulePath;
    }
    return src.absoluteFilePath(module + ".rs");
}

void writeRustTypes(const Configuration& conf, QTextStream& r) {
    bool hasOption = false;
    bool hasString = false;
    bool hasByteArray = false;
    bool hasListOrTree = false;

    for (auto o: conf.objects) {
        hasListOrTree |= o.type != ObjectType::Object;
        for (auto p: o.properties) {
            hasOption |= p.optional;
            hasString |= p.type.type == BindingType::QString;
            hasByteArray |= p.type.type == BindingType::QByteArray;
        }
        for (auto p: o.itemProperties) {
            hasOption |= p.optional;
            hasString |= p.type.type == BindingType::QString;
            hasByteArray |= p.type.type == BindingType::QByteArray;
        }
        for (auto f: o.functions) {
            hasString |= f.type.type == BindingType::QString;
            hasByteArray |= f.type.type == BindingType::QByteArray;
            for (auto a: f.args) {
                hasString |= a.type.type == BindingType::QString;
                hasByteArray |= a.type.type == BindingType::QByteArray;
            }
        }
    }

    if (hasOption || hasListOrTree) {
        r << R"(

#[repr(C)]
pub struct COption<T> {
    data: T,
    some: bool,
}

impl<T> COption<T> {
    #![allow(dead_code)]
    fn into(self) -> Option<T> {
        if self.some {
            Some(self.data)
        } else {
            None
        }
    }
}

impl<T> From<Option<T>> for COption<T>
where
    T: Default,
{
    fn from(t: Option<T>) -> COption<T> {
        if let Some(v) = t {
            COption {
                data: v,
                some: true,
            }
        } else {
            COption {
                data: T::default(),
                some: false,
            }
        }
    }
}
)";
    }
    if (hasString) {
        r << R"(

pub enum QString {}

fn set_string_from_utf16(s: &mut String, str: *const c_ushort, len: c_int) {
    let utf16 = unsafe { slice::from_raw_parts(str, to_usize(len)) };
    let characters = decode_utf16(utf16.iter().cloned())
        .into_iter()
        .map(|r| r.unwrap());
    s.clear();
    s.extend(characters);
}

)";
                 }
    if (hasByteArray) {
        r << R"(

pub enum QByteArray {}
)";
    }
    if (hasListOrTree) {
        r << R"(

#[repr(C)]
#[derive(PartialEq, Eq, Debug)]
pub enum SortOrder {
    Ascending = 0,
    Descending = 1,
}

#[repr(C)]
pub struct QModelIndex {
    row: c_int,
    internal_id: usize,
}
)";
    }

    if (hasString || hasByteArray || hasListOrTree) {
        r << R"(

fn to_usize(n: c_int) -> usize {
    if n < 0 {
        panic!("Cannot cast {} to usize", n);
    }
    n as usize
}

)";
    }

    if (hasString || hasByteArray || hasListOrTree) {
        r << R"(
fn to_c_int(n: usize) -> c_int {
    if n > c_int::max_value() as usize {
        panic!("Cannot cast {} to c_int", n);
    }
    n as c_int
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
use libc::{c_char, c_ushort, c_int};
use std::slice;
use std::char::decode_utf16;

use std::sync::{Arc, Mutex};
use std::ptr::null;

use %1::*;
)").arg(conf.implementationModule);

    writeRustTypes(conf, r);

    for (auto object: conf.objects) {
        writeRustInterfaceObject(r, object, conf);
    }
}

void writeRustImplementationObject(QTextStream& r, const Object& o) {
    const QString lcname(snakeCase(o.name));
    if (o.type != ObjectType::Object) {
        r << "#[derive(Default, Clone)]\n";
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
    } else if (o.type == ObjectType::Tree) {
        modelStruct = ", model: " + o.name + "Tree";
        r << QString("    model: %1Tree,\n").arg(o.name);
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        r << QString("    %1: %2,\n").arg(lc, rustType(p));
    }
    if (o.type != ObjectType::Object) {
        r << QString("    list: Vec<%1Item>,\n").arg(o.name);
    }
    r << "}\n\n";
    for (const Property& p: o.properties) {
        if (p.type.type == BindingType::Object) {
            modelStruct += ", " + p.name + ": " + p.type.name;
        }
    }
    r << QString(R"(impl %1Trait for %1 {
    fn new(emit: %1Emitter%2) -> %1 {
        %1 {
            emit: emit,
)").arg(o.name, modelStruct);
    if (o.type != ObjectType::Object) {
        r << QString("            model: model,\n");
    }
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        if (p.type.type == BindingType::Object) {
            r << QString("            %1: %1,\n").arg(lc);
        } else {
            r << QString("            %1: %2,\n").arg(lc, rustTypeInit(p));
        }
    }
    r << QString(R"(        }
    }
    fn emit(&self) -> &%1Emitter {
        &self.emit
    }
)").arg(o.name);
    for (const Property& p: o.properties) {
        const QString lc(snakeCase(p.name));
        if (p.type.type == BindingType::Object) {
            r << QString(R"(    fn %1(&self) -> &%2 {
        &self.%1
    }
    fn %1_mut(&mut self) -> &mut %2 {
        &mut self.%1
    }
)").arg(lc, rustReturnType(p));
        } else {
            r << QString("    fn %1(&self) -> %2 {\n").arg(lc, rustReturnType(p));
            if (p.type.isComplex()) {
                if (p.optional) {
/*
                    if (rustType(p) == "Option<String>") {
                        r << QString("        self.%1.as_ref().map(|p|p.as_str())\n").arg(lc);
                    } else {
                    }
*/
                    r << QString("        self.%1.as_ref().map(|p|&p[..])\n").arg(lc);
                } else {
                    r << QString("        &self.%1\n").arg(lc);
                }
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
    }
    if (o.type == ObjectType::List) {
        r << "    fn row_count(&self) -> usize {\n        self.list.len()\n    }\n";
    } else if (o.type == ObjectType::Tree) {
        r << R"(    fn row_count(&self, item: Option<usize>) -> usize {
        self.list.len()
    }
    fn index(&self, item: Option<usize>, row: usize) -> usize {
        0
    }
    fn parent(&self, index: usize) -> Option<usize> {
        None
    }
    fn row(&self, index: usize) -> usize {
        item
    }
)";
    }
    if (o.type != ObjectType::Object) {
        QString index;
        if (o.type == ObjectType::Tree) {
            index = ", index: usize";
        }
        for (auto ip: o.itemProperties) {
            const QString lc(snakeCase(ip.name));
            r << QString("    fn %1(&self, index: usize) -> %2 {\n")
                    .arg(lc, rustReturnType(ip));
            if (ip.type.isComplex()) {
                r << "        &self.list[index]." << lc << "\n";
            } else {
                r << "        self.list[index]." << lc << "\n";
            }
            r << "    }\n";
            if (ip.write) {
                r << QString("    fn set_%1(&mut self, index: usize, v: %2) -> bool {\n")
                        .arg(snakeCase(ip.name), rustType(ip));
                r << "        self.list[index]." << lc << " = v;\n";
                r << "        true\n";
                r << "    }\n";
            }
        }
    }
    r << "}\n\n";
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
