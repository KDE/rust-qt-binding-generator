//! `cpp` is the module that generates the cpp code for the bindings

use configuration::*;
use configuration_private::*;
use std::io::{Result, Write};
use util::{snake_case, write_if_different};

fn property_type(p: &ItemProperty) -> String {
    if p.optional && !p.item_property_type.is_complex() {
        return "QVariant".into();
    }
    p.type_name().to_string()
}

fn upper_initial(name: &str) -> String {
    format!("{}{}", &name[..1].to_uppercase(), &name[1..])
}

fn lower_initial(name: &str) -> String {
    format!("{}{}", &name[..1].to_lowercase(), &name[1..])
}

fn write_property(name: &str) -> String {
    format!("WRITE set{} ", upper_initial(name))
}

fn base_type(o: &Object) -> &str {
    if o.object_type != ObjectType::Object {
        return "QAbstractItemModel";
    }
    "QObject"
}

fn model_is_writable(o: &Object) -> bool {
    let mut write = false;
    for p in o.item_properties.values() {
        write |= p.write;
    }
    write
}

fn write_header_item_model(h: &mut Vec<u8>, o: &Object) -> Result<()> {
    writeln!(
        h,
        "
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    int role(const char* name) const;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;"
    )?;
    if model_is_writable(o) {
        writeln!(
            h,
            "    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;"
        )?;
    }
    for (name, ip) in &o.item_properties {
        let r = property_type(ip);
        let rw = if r == "QVariant" || ip.item_property_type.is_complex() {
            format!("const {}&", r)
        } else {
            r.clone()
        };
        if o.object_type == ObjectType::List {
            writeln!(h, "    Q_INVOKABLE {} {}(int row) const;", r, name)?;
            if ip.write {
                writeln!(
                    h,
                    "    Q_INVOKABLE bool set{}(int row, {} value);",
                    upper_initial(name),
                    rw
                )?;
            }
        } else {
            writeln!(
                h,
                "    Q_INVOKABLE {} {}(const QModelIndex& index) const;",
                r, name
            )?;
            if ip.write {
                writeln!(
                    h,
                    "    Q_INVOKABLE bool set{}(const QModelIndex& index, {} value);",
                    upper_initial(name),
                    rw
                )?;
            }
        }
    }
    writeln!(
        h,
        "
Q_SIGNALS:
    // new data is ready to be made available to the model with fetchMore()
    void newDataReady(const QModelIndex &parent) const;
private:
    QHash<QPair<int,Qt::ItemDataRole>, QVariant> m_headerData;
    void initHeaderData();
    void updatePersistentIndexes();"
    )?;
    Ok(())
}

fn write_header_object(h: &mut Vec<u8>, o: &Object, conf: &Config) -> Result<()> {
    writeln!(
        h,
        "
class {} : public {}
{{
    Q_OBJECT",
        o.name,
        base_type(o)
    )?;
    for object in conf.objects.values() {
        if object.contains_object() && o.name != object.name {
            writeln!(h, "    friend class {};", object.name)?;
        }
    }
    writeln!(
        h,
        "public:
    class Private;
private:"
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            writeln!(h, "    {}* const m_{};", p.type_name(), name)?;
        }
    }
    writeln!(
        h,
        "    Private * m_d;
    bool m_ownsPrivate;"
    )?;
    for (name, p) in &o.properties {
        let mut t = if p.optional && !p.is_complex() {
            "QVariant"
        } else {
            p.type_name()
        }
        .to_string();
        if p.is_object() {
            t.push('*');
        }
        writeln!(
            h,
            "    Q_PROPERTY({0} {1} READ {1} {2}NOTIFY {1}Changed FINAL)",
            t,
            name,
            if p.write {
                write_property(name)
            } else {
                String::new()
            }
        )?;
    }
    writeln!(
        h,
        "    explicit {}(bool owned, QObject *parent);
public:
    explicit {0}(QObject *parent = nullptr);
    ~{0}();",
        o.name
    )?;
    for (name, p) in &o.properties {
        if p.is_object() {
            writeln!(h, "    const {}* {}() const;", p.type_name(), name)?;
            writeln!(h, "    {}* {}();", p.type_name(), name)?;
        } else {
            let (t, t2) = if p.optional && !p.is_complex() {
                ("QVariant", "const QVariant&")
            } else {
                (p.type_name(), p.property_type.cpp_set_type())
            };
            writeln!(h, "    {} {}() const;", t, name)?;
            if p.write {
                writeln!(h, "    void set{}({} v);", upper_initial(name), t2)?;
            }
        }
    }
    for (name, f) in &o.functions {
        write!(h, "    Q_INVOKABLE {} {}(", f.return_type.name(), name)?;
        for (i, a) in f.arguments.iter().enumerate() {
            if i != 0 {
                write!(h, ", ")?;
            }
            write!(h, "{} {}", a.argument_type.cpp_set_type(), a.name)?;
        }
        writeln!(h, "){};", if f.mutable { "" } else { " const" })?;
    }
    if base_type(o) == "QAbstractItemModel" {
        write_header_item_model(h, o)?;
    }
    writeln!(h, "Q_SIGNALS:")?;
    for name in o.properties.keys() {
        writeln!(h, "    void {}Changed();", name)?;
    }
    writeln!(h, "}};")?;
    Ok(())
}

fn is_column_write(o: &Object, col: usize) -> bool {
    o.item_properties
        .values()
        .any(|ip| ip.write && (col == 0 || (ip.roles.len() > col && !ip.roles[col].is_empty())))
}

fn write_function_c_decl(
    w: &mut Vec<u8>,
    (name, f): (&String, &Function),
    lcname: &str,
    o: &Object,
) -> Result<()> {
    let lc = snake_case(name);
    write!(w, "    ")?;
    if f.return_type.is_complex() {
        write!(w, "void")?;
    } else {
        write!(w, "{}", f.type_name())?;
    }
    let name = format!("{}_{}", lcname, lc);
    write!(
        w,
        " {}({}{}::Private*",
        name,
        if f.mutable { "" } else { "const " },
        o.name
    )?;

    // write all the input arguments, for QString and QByteArray, write
    // pointers to their content and the length
    for a in &f.arguments {
        if a.type_name() == "QString" {
            write!(w, ", const ushort*, int")?;
        } else if a.type_name() == "QByteArray" {
            write!(w, ", const char*, int")?;
        } else {
            write!(w, ", {}", a.type_name())?;
        }
    }
    // If the return type is QString or QByteArray, append a pointer to the
    // variable that will be set to the argument list. Also add a setter
    // function.
    if f.return_type.name() == "QString" {
        write!(w, ", QString*, qstring_set")?;
    } else if f.return_type.name() == "QByteArray" {
        write!(w, ", QByteArray*, qbytearray_set")?;
    }
    writeln!(w, ");")?;
    Ok(())
}

fn write_object_c_decl(w: &mut Vec<u8>, o: &Object, conf: &Config) -> Result<()> {
    let lcname = snake_case(&o.name);
    write!(w, "    {}::Private* {}_new(", o.name, lcname)?;
    constructor_args_decl(w, o, conf)?;
    writeln!(w, ");")?;
    writeln!(w, "    void {}_free({}::Private*);", lcname, o.name)?;
    for (name, p) in &o.properties {
        let base = format!("{}_{}", lcname, snake_case(name));
        if p.is_object() {
            writeln!(
                w,
                "    {}::Private* {}_get(const {}::Private*);",
                p.type_name(),
                base,
                o.name
            )?;
        } else if p.is_complex() {
            writeln!(
                w,
                "    void {}_get(const {}::Private*, {});",
                base,
                o.name,
                p.c_get_type()
            )?;
        } else if p.optional {
            writeln!(
                w,
                "    option_{} {}_get(const {}::Private*);",
                p.type_name(),
                base,
                o.name
            )?;
        } else {
            writeln!(
                w,
                "    {} {}_get(const {}::Private*);",
                p.type_name(),
                base,
                o.name
            )?;
        }
        if p.write {
            let mut t = p.property_type.c_set_type();
            if t == "qstring_t" {
                t = "const ushort *str, int len";
            } else if t == "qbytearray_t" {
                t = "const char* bytes, int len";
            }
            writeln!(w, "    void {}_set({}::Private*, {});", base, o.name, t)?;
            if p.optional {
                writeln!(w, "    void {}_set_none({}::Private*);", base, o.name)?;
            }
        }
    }
    for f in &o.functions {
        write_function_c_decl(w, f, &lcname, o)?;
    }
    Ok(())
}

fn initialize_members_zero(w: &mut Vec<u8>, o: &Object) -> Result<()> {
    for (name, p) in &o.properties {
        if p.is_object() {
            writeln!(w, "    m_{}(new {}(false, this)),", name, p.type_name())?;
        }
    }
    Ok(())
}

fn initialize_members(w: &mut Vec<u8>, prefix: &str, o: &Object, conf: &Config) -> Result<()> {
    for (name, p) in &o.properties {
        if let Type::Object(object) = &p.property_type {
            writeln!(
                w,
                "    {}m_{}->m_d = {}_{}_get({0}m_d);",
                prefix,
                name,
                snake_case(&o.name),
                snake_case(name)
            )?;
            initialize_members(w, &format!("m_{}->", name), object, conf)?;
        }
    }
    Ok(())
}

fn connect(w: &mut Vec<u8>, d: &str, o: &Object, conf: &Config) -> Result<()> {
    for (name, p) in &o.properties {
        if let Type::Object(object) = &p.property_type {
            connect(w, &format!("{}->m_{}", d, name), object, conf)?;
        }
    }
    if o.object_type != ObjectType::Object {
        writeln!(
            w,
            "    connect({}, &{1}::newDataReady, {0}, [this](const QModelIndex& i) {{
        {0}->fetchMore(i);
    }}, Qt::QueuedConnection);",
            d, o.name
        )?;
    }
    Ok(())
}

fn write_cpp_object_properties(w: &mut Vec<u8>, o: &Object, lcname: &str) -> Result<()> {
    for (name, p) in &o.properties {
        let base = format!("{}_{}", lcname, snake_case(name));
        if p.is_object() {
            writeln!(
                w,
                "const {}* {}::{}() const
{{
    return m_{2};
}}
{0}* {1}::{2}()
{{
    return m_{2};
}}",
                p.type_name(),
                o.name,
                name
            )?;
        } else if p.is_complex() {
            writeln!(
                w,
                "{} {}::{}() const
{{
    {0} v;
    {3}_get(m_d, &v, set_{4});
    return v;
}}",
                p.type_name(),
                o.name,
                name,
                base,
                p.type_name().to_lowercase()
            )?;
        } else if p.optional {
            writeln!(
                w,
                "QVariant {}::{}() const
{{
    QVariant v;
    auto r = {2}_get(m_d);
    if (r.some) {{
        v.setValue(r.value);
    }}
    return r;
}}",
                o.name, name, base
            )?;
        } else {
            writeln!(
                w,
                "{} {}::{}() const
{{
    return {}_get(m_d);
}}",
                p.type_name(),
                o.name,
                name,
                base
            )?;
        }
        if p.write {
            let t = if p.optional && !p.is_complex() {
                "const QVariant&"
            } else {
                p.property_type.cpp_set_type()
            };
            writeln!(w, "void {}::set{}({} v) {{", o.name, upper_initial(name), t)?;
            if p.optional {
                if p.is_complex() {
                    writeln!(w, "    if (v.isNull()) {{")?;
                } else {
                    writeln!(
                        w,
                        "    if (v.isNull() || !v.canConvert<{}>()) {{",
                        p.type_name()
                    )?;
                }
                writeln!(w, "        {}_set_none(m_d);", base)?;
                writeln!(w, "    }} else {{")?;
                if p.type_name() == "QString" {
                    writeln!(
                        w,
                        "    {}_set(m_d, reinterpret_cast<const ushort*>(v.data()), v.size());",
                        base
                    )?;
                } else if p.type_name() == "QByteArray" {
                    writeln!(w, "    {}_set(m_d, v.data(), v.size());", base)?;
                } else if p.optional {
                    writeln!(
                        w,
                        "        {}_set(m_d, v.value<{}>());",
                        base,
                        p.type_name()
                    )?;
                } else {
                    writeln!(w, "        {}_set(m_d, v);", base)?;
                }
                writeln!(w, "    }}")?;
            } else if p.type_name() == "QString" {
                writeln!(
                    w,
                    "    {}_set(m_d, reinterpret_cast<const ushort*>(v.data()), v.size());",
                    base
                )?;
            } else if p.type_name() == "QByteArray" {
                writeln!(w, "    {}_set(m_d, v.data(), v.size());", base)?;
            } else {
                writeln!(w, "    {}_set(m_d, v);", base)?;
            }
            writeln!(w, "}}")?;
        }
    }
    Ok(())
}

fn write_cpp_object(w: &mut Vec<u8>, o: &Object, conf: &Config) -> Result<()> {
    let lcname = snake_case(&o.name);
    writeln!(
        w,
        "{}::{0}(bool /*owned*/, QObject *parent):
    {}(parent),",
        o.name,
        base_type(o)
    )?;
    initialize_members_zero(w, o)?;
    writeln!(
        w,
        "    m_d(nullptr),
    m_ownsPrivate(false)
{{"
    )?;
    if o.object_type != ObjectType::Object {
        writeln!(w, "    initHeaderData();")?;
    }
    writeln!(
        w,
        "}}

{}::{0}(QObject *parent):
    {}(parent),",
        o.name,
        base_type(o)
    )?;
    initialize_members_zero(w, o)?;
    write!(w, "    m_d({}_new(this", lcname)?;
    constructor_args(w, "", o, conf)?;
    writeln!(
        w,
        ")),
    m_ownsPrivate(true)
{{"
    )?;
    initialize_members(w, "", o, conf)?;
    connect(w, "this", o, conf)?;
    if o.object_type != ObjectType::Object {
        writeln!(w, "    initHeaderData();")?;
    }
    writeln!(
        w,
        "}}

{}::~{0}() {{
    if (m_ownsPrivate) {{
        {1}_free(m_d);
    }}
}}",
        o.name, lcname
    )?;
    if o.object_type != ObjectType::Object {
        writeln!(w, "void {}::initHeaderData() {{", o.name)?;
        for col in 0..o.column_count() {
            for (name, ip) in &o.item_properties {
                let empty = Vec::new();
                let roles = ip.roles.get(col).unwrap_or(&empty);
                if roles.contains(&"display".to_string()) {
                    writeln!(
                        w,
                        "    m_headerData.insert(qMakePair({}, Qt::DisplayRole), QVariant(\"{}\"));",
                        col,
                        name
                    )?;
                }
            }
        }
        writeln!(w, "}}")?;
    }

    write_cpp_object_properties(w, o, &lcname)?;

    for (name, f) in &o.functions {
        let base = format!("{}_{}", lcname, snake_case(name));
        write!(w, "{} {}::{}(", f.type_name(), o.name, name)?;
        for (i, a) in f.arguments.iter().enumerate() {
            write!(
                w,
                "{} {}{}",
                a.argument_type.cpp_set_type(),
                a.name,
                if i + 1 < f.arguments.len() { ", " } else { "" }
            )?;
        }
        writeln!(w, "){}\n{{", if f.mutable { "" } else { " const" })?;
        let mut arg_list = String::new();
        for a in &f.arguments {
            if a.type_name() == "QString" {
                arg_list.push_str(&format!(", {}.utf16(), {0}.size()", a.name));
            } else if a.type_name() == "QByteArray" {
                arg_list.push_str(&format!(", {}.data(), {0}.size()", a.name));
            } else {
                arg_list.push_str(&format!(", {}", a.name));
            }
        }
        if f.return_type.name() == "QString" {
            writeln!(
                w,
                "    {} s;
    {}(m_d{}, &s, set_qstring);
    return s;",
                f.type_name(),
                base,
                arg_list
            )?;
        } else if f.return_type.name() == "QByteArray" {
            writeln!(
                w,
                "    {} s;
    {}(m_d{}, &s, set_qbytearray);
    return s;",
                f.type_name(),
                base,
                arg_list
            )?;
        } else {
            writeln!(w, "    return {}(m_d{});", base, arg_list)?;
        }
        writeln!(w, "}}")?;
    }
    Ok(())
}

fn role_name(role: &str) -> String {
    match role {
        "display" => "DisplayRole".into(),
        "decoration" => "DecorationRole".into(),
        "edit" => "EditRole".into(),
        "toolTip" => "ToolTipRole".into(),
        "statustip" => "StatusTipRole".into(),
        "whatsthis" => "WhatsThisRole".into(),
        _ => panic!("Unknown role {}", role),
    }
}

fn write_model_getter_setter(
    w: &mut Vec<u8>,
    index: &str,
    name: &str,
    ip: &ItemProperty,
    o: &Object,
) -> Result<()> {
    let lcname = snake_case(&o.name);
    let mut idx = index;

    // getter
    let mut r = property_type(ip);
    if o.object_type == ObjectType::List {
        idx = ", row";
        writeln!(w, "{} {}::{}(int row) const\n{{", r, o.name, name)?;
    } else {
        writeln!(
            w,
            "{} {}::{}(const QModelIndex& index) const\n{{",
            r, o.name, name
        )?;
    }
    if ip.type_name() == "QString" {
        writeln!(w, "    QString s;")?;
        writeln!(
            w,
            "    {}_data_{}(m_d{}, &s, set_{});",
            lcname,
            snake_case(name),
            idx,
            ip.type_name().to_lowercase()
        )?;
        writeln!(w, "    return s;")?;
    } else if ip.type_name() == "QByteArray" {
        writeln!(w, "    QByteArray b;")?;
        writeln!(
            w,
            "    {}_data_{}(m_d{}, &b, set_{});",
            lcname,
            snake_case(name),
            idx,
            ip.type_name().to_lowercase()
        )?;
        writeln!(w, "    return b;")?;
    } else if ip.optional {
        writeln!(w, "    QVariant v;")?;
        writeln!(
            w,
            "    v = {}_data_{}(m_d{});",
            lcname,
            snake_case(name),
            idx
        )?;
        writeln!(w, "    return v;")?;
    } else {
        writeln!(
            w,
            "    return {}_data_{}(m_d{});",
            lcname,
            snake_case(name),
            idx
        )?;
    }
    writeln!(w, "}}\n")?;
    if !ip.write {
        return Ok(());
    }

    //setter
    if r == "QVariant" || ip.is_complex() {
        r = format!("const {}&", r);
    }
    if o.object_type == ObjectType::List {
        idx = ", row";
        writeln!(
            w,
            "bool {}::set{}(int row, {} value)\n{{",
            o.name,
            upper_initial(name),
            r
        )?;
    } else {
        writeln!(
            w,
            "bool {}::set{}(const QModelIndex& index, {} value)\n{{",
            o.name,
            upper_initial(name),
            r
        )?;
    }
    writeln!(w, "    bool set = false;")?;
    if ip.optional {
        let mut test = "value.isNull()".to_string();
        if !ip.is_complex() {
            test += " || !value.isValid()";
        }
        writeln!(w, "    if ({}) {{", test)?;
        writeln!(
            w,
            "        set = {}_set_data_{}_none(m_d{});",
            lcname,
            snake_case(name),
            idx
        )?;
        writeln!(w, "    }} else {{")?;
    }
    if ip.optional && !ip.is_complex() {
        writeln!(
            w,
            "    if (!value.canConvert(qMetaTypeId<{}>())) {{
        return false;
    }}",
            ip.type_name()
        )?;
        writeln!(
            w,
            "    set = {}_set_data_{}(m_d{}, value.value<{}>());",
            lcname,
            snake_case(name),
            idx,
            ip.type_name()
        )?;
    } else {
        let mut val = "value";
        if ip.is_complex() {
            if ip.type_name() == "QString" {
                val = "value.utf16(), value.length()";
            } else {
                val = "value.data(), value.length()";
            }
        }
        writeln!(
            w,
            "    set = {}_set_data_{}(m_d{}, {});",
            lcname,
            snake_case(name),
            idx,
            val
        )?;
    }
    if ip.optional {
        writeln!(w, "    }}")?;
    }
    if o.object_type == ObjectType::List {
        writeln!(
            w,
            "    if (set) {{
        QModelIndex index = createIndex(row, 0, row);
        Q_EMIT dataChanged(index, index);
    }}
    return set;
}}
"
        )?;
    } else {
        writeln!(
            w,
            "    if (set) {{
        Q_EMIT dataChanged(index, index);
    }}
    return set;
}}
"
        )?;
    }
    Ok(())
}

fn write_cpp_model(w: &mut Vec<u8>, o: &Object) -> Result<()> {
    let lcname = snake_case(&o.name);
    let (index_decl, index) = if o.object_type == ObjectType::Tree {
        (", quintptr", ", index.internalId()")
    } else {
        (", int", ", index.row()")
    };
    writeln!(w, "extern \"C\" {{")?;

    for (name, ip) in &o.item_properties {
        if ip.is_complex() {
            writeln!(
                w,
                "    void {}_data_{}(const {}::Private*{}, {});",
                lcname,
                snake_case(name),
                o.name,
                index_decl,
                ip.c_get_type()
            )?;
        } else {
            writeln!(
                w,
                "    {} {}_data_{}(const {}::Private*{});",
                ip.cpp_set_type(),
                lcname,
                snake_case(name),
                o.name,
                index_decl
            )?;
        }
        if ip.write {
            let a = format!("    bool {}_set_data_{}", lcname, snake_case(name));
            let b = format!("({}::Private*{}", o.name, index_decl);
            if ip.type_name() == "QString" {
                writeln!(w, "{}{}, const ushort* s, int len);", a, b)?;
            } else if ip.type_name() == "QByteArray" {
                writeln!(w, "{}{}, const char* s, int len);", a, b)?;
            } else {
                writeln!(w, "{}{}, {});", a, b, ip.c_set_type())?;
            }
            if ip.optional {
                writeln!(w, "{}_none{});", a, b)?;
            }
        }
    }
    writeln!(
        w,
        "    void {}_sort({}::Private*, unsigned char column, Qt::SortOrder order = Qt::AscendingOrder);",
        lcname,
        o.name
    )?;
    if o.object_type == ObjectType::List {
        writeln!(
            w,
            "
    int {1}_row_count(const {0}::Private*);
    bool {1}_insert_rows({0}::Private*, int, int);
    bool {1}_remove_rows({0}::Private*, int, int);
    bool {1}_can_fetch_more(const {0}::Private*);
    void {1}_fetch_more({0}::Private*);
}}
int {0}::columnCount(const QModelIndex &parent) const
{{
    return (parent.isValid()) ? 0 : {2};
}}

bool {0}::hasChildren(const QModelIndex &parent) const
{{
    return rowCount(parent) > 0;
}}

int {0}::rowCount(const QModelIndex &parent) const
{{
    return (parent.isValid()) ? 0 : {1}_row_count(m_d);
}}

bool {0}::insertRows(int row, int count, const QModelIndex &)
{{
    return {1}_insert_rows(m_d, row, count);
}}

bool {0}::removeRows(int row, int count, const QModelIndex &)
{{
    return {1}_remove_rows(m_d, row, count);
}}

QModelIndex {0}::index(int row, int column, const QModelIndex &parent) const
{{
    if (!parent.isValid() && row >= 0 && row < rowCount(parent) && column >= 0 && column < {2}) {{
        return createIndex(row, column, (quintptr)row);
    }}
    return QModelIndex();
}}

QModelIndex {0}::parent(const QModelIndex &) const
{{
    return QModelIndex();
}}

bool {0}::canFetchMore(const QModelIndex &parent) const
{{
    return (parent.isValid()) ? 0 : {1}_can_fetch_more(m_d);
}}

void {0}::fetchMore(const QModelIndex &parent)
{{
    if (!parent.isValid()) {{
        {1}_fetch_more(m_d);
    }}
}}
void {0}::updatePersistentIndexes() {{}}",
            o.name,
            lcname,
            o.column_count()
        )?;
    } else {
        writeln!(
            w,
            "
    int {1}_row_count(const {0}::Private*, option_quintptr);
    bool {1}_can_fetch_more(const {0}::Private*, option_quintptr);
    void {1}_fetch_more({0}::Private*, option_quintptr);
    quintptr {1}_index(const {0}::Private*, option_quintptr, int);
    qmodelindex_t {1}_parent(const {0}::Private*, quintptr);
    int {1}_row(const {0}::Private*, quintptr);
    option_quintptr {1}_check_row(const {0}::Private*, quintptr, int);
}}
int {0}::columnCount(const QModelIndex &) const
{{
    return {2};
}}

bool {0}::hasChildren(const QModelIndex &parent) const
{{
    return rowCount(parent) > 0;
}}

int {0}::rowCount(const QModelIndex &parent) const
{{
    if (parent.isValid() && parent.column() != 0) {{
        return 0;
    }}
    const option_quintptr rust_parent = {{
        parent.internalId(),
        parent.isValid()
    }};
    return {1}_row_count(m_d, rust_parent);
}}

bool {0}::insertRows(int, int, const QModelIndex &)
{{
    return false; // not supported yet
}}

bool {0}::removeRows(int, int, const QModelIndex &)
{{
    return false; // not supported yet
}}

QModelIndex {0}::index(int row, int column, const QModelIndex &parent) const
{{
    if (row < 0 || column < 0 || column >= {2}) {{
        return QModelIndex();
    }}
    if (parent.isValid() && parent.column() != 0) {{
        return QModelIndex();
    }}
    if (row >= rowCount(parent)) {{
        return QModelIndex();
    }}
    const option_quintptr rust_parent = {{
        parent.internalId(),
        parent.isValid()
    }};
    const quintptr id = {1}_index(m_d, rust_parent, row);
    return createIndex(row, column, id);
}}

QModelIndex {0}::parent(const QModelIndex &index) const
{{
    if (!index.isValid()) {{
        return QModelIndex();
    }}
    const qmodelindex_t parent = {1}_parent(m_d, index.internalId());
    return parent.row >= 0 ?createIndex(parent.row, 0, parent.id) :QModelIndex();
}}

bool {0}::canFetchMore(const QModelIndex &parent) const
{{
    if (parent.isValid() && parent.column() != 0) {{
        return false;
    }}
    const option_quintptr rust_parent = {{
        parent.internalId(),
        parent.isValid()
    }};
    return {1}_can_fetch_more(m_d, rust_parent);
}}

void {0}::fetchMore(const QModelIndex &parent)
{{
    const option_quintptr rust_parent = {{
        parent.internalId(),
        parent.isValid()
    }};
    {1}_fetch_more(m_d, rust_parent);
}}
void {0}::updatePersistentIndexes() {{
    const auto from = persistentIndexList();
    auto to = from;
    auto len = to.size();
    for (int i = 0; i < len; ++i) {{
        auto index = to.at(i);
        auto row = {1}_check_row(m_d, index.internalId(), index.row());
        if (row.some) {{
            to[i] = createIndex(row.value, index.column(), index.internalId());
        }} else {{
            to[i] = QModelIndex();
        }}
    }}
    changePersistentIndexList(from, to);
}}",
            o.name,
            lcname,
            o.column_count()
        )?;
    }
    writeln!(
        w,
        "
void {0}::sort(int column, Qt::SortOrder order)
{{
    {1}_sort(m_d, column, order);
}}
Qt::ItemFlags {0}::flags(const QModelIndex &i) const
{{
    auto flags = QAbstractItemModel::flags(i);",
        o.name, lcname
    )?;
    for col in 0..o.column_count() {
        if is_column_write(o, col) {
            writeln!(w, "    if (i.column() == {}) {{", col)?;
            writeln!(w, "        flags |= Qt::ItemIsEditable;\n    }}")?;
        }
    }
    writeln!(w, "    return flags;\n}}\n")?;
    for ip in &o.item_properties {
        write_model_getter_setter(w, index, ip.0, ip.1, o)?;
    }
    writeln!(
        w,
        "QVariant {}::data(const QModelIndex &index, int role) const
{{
    Q_ASSERT(rowCount(index.parent()) > index.row());
    switch (index.column()) {{",
        o.name
    )?;

    for col in 0..o.column_count() {
        writeln!(w, "    case {}:", col)?;
        writeln!(w, "        switch (role) {{")?;
        for (i, (name, ip)) in o.item_properties.iter().enumerate() {
            let empty = Vec::new();
            let roles = ip.roles.get(col).unwrap_or(&empty);
            if col > 0 && roles.is_empty() {
                continue;
            }
            for role in roles {
                writeln!(w, "        case Qt::{}:", role_name(role))?;
            }
            writeln!(w, "        case Qt::UserRole + {}:", i)?;
            let ii = if o.object_type == ObjectType::List {
                ".row()"
            } else {
                ""
            };
            if ip.optional && !ip.is_complex() {
                writeln!(w, "            return {}(index{});", name, ii)?;
            } else if ip.optional {
                writeln!(
                    w,
                    "            return cleanNullQVariant(QVariant::fromValue({}(index{})));",
                    name, ii
                )?;
            } else {
                writeln!(
                    w,
                    "            return QVariant::fromValue({}(index{}));",
                    name, ii
                )?;
            }
        }
        writeln!(w, "        }}\n        break;")?;
    }
    writeln!(
        w,
        "    }}
    return QVariant();
}}

int {}::role(const char* name) const {{
    auto names = roleNames();
    auto i = names.constBegin();
    while (i != names.constEnd()) {{
        if (i.value() == name) {{
            return i.key();
        }}
        ++i;
    }}
    return -1;
}}
QHash<int, QByteArray> {0}::roleNames() const {{
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();",
        o.name
    )?;
    for (i, (name, _)) in o.item_properties.iter().enumerate() {
        writeln!(w, "    names.insert(Qt::UserRole + {}, \"{}\");", i, name)?;
    }
    writeln!(
        w,
        "    return names;
}}
QVariant {0}::headerData(int section, Qt::Orientation orientation, int role) const
{{
    if (orientation != Qt::Horizontal) {{
        return QVariant();
    }}
    return m_headerData.value(qMakePair(section, (Qt::ItemDataRole)role), role == Qt::DisplayRole ?QString::number(section + 1) :QVariant());
}}

bool {0}::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{{
    if (orientation != Qt::Horizontal) {{
        return false;
    }}
    m_headerData.insert(qMakePair(section, (Qt::ItemDataRole)role), value);
    return true;
}}
",
        o.name
    )?;
    if model_is_writable(o) {
        writeln!(
            w,
            "bool {}::setData(const QModelIndex &index, const QVariant &value, int role)\n{{",
            o.name
        )?;
        for col in 0..o.column_count() {
            if !is_column_write(o, col) {
                continue;
            }
            writeln!(w, "    if (index.column() == {}) {{", col)?;
            for (i, (name, ip)) in o.item_properties.iter().enumerate() {
                if !ip.write {
                    continue;
                }
                let empty = Vec::new();
                let roles = ip.roles.get(col).unwrap_or(&empty);
                if col > 0 && roles.is_empty() {
                    continue;
                }
                write!(w, "        if (")?;
                for role in roles {
                    write!(w, "role == Qt::{} || ", role_name(role))?;
                }
                writeln!(w, "role == Qt::UserRole + {}) {{", i)?;
                let ii = if o.object_type == ObjectType::List {
                    ".row()"
                } else {
                    ""
                };
                if ip.optional && !ip.is_complex() {
                    writeln!(
                        w,
                        "            return set{}(index{}, value);",
                        upper_initial(name),
                        ii
                    )?;
                } else {
                    let pre = if ip.optional {
                        "!value.isValid() || value.isNull() ||"
                    } else {
                        ""
                    };
                    writeln!(
                        w,
                        "            if ({}value.canConvert(qMetaTypeId<{}>())) {{",
                        pre,
                        ip.type_name()
                    )?;
                    writeln!(
                        w,
                        "                return set{}(index{}, value.value<{}>());",
                        upper_initial(name),
                        ii,
                        ip.type_name()
                    )?;
                    writeln!(w, "            }}")?;
                }
                writeln!(w, "        }}")?;
            }
            writeln!(w, "    }}")?;
        }
        writeln!(w, "    return false;\n}}\n")?;
    }
    Ok(())
}

fn constructor_args_decl(w: &mut Vec<u8>, o: &Object, conf: &Config) -> Result<()> {
    write!(w, "{}*", o.name)?;
    for p in o.properties.values() {
        if let Type::Object(object) = &p.property_type {
            write!(w, ", ")?;
            constructor_args_decl(w, object, conf)?;
        } else {
            write!(w, ", void (*)({}*)", o.name)?;
        }
    }
    if o.object_type == ObjectType::List {
        write!(
            w,
            ",
        void (*)(const {}*),
        void (*)({0}*),
        void (*)({0}*),
        void (*)({0}*, quintptr, quintptr),
        void (*)({0}*),
        void (*)({0}*),
        void (*)({0}*, int, int),
        void (*)({0}*),
        void (*)({0}*, int, int, int),
        void (*)({0}*),
        void (*)({0}*, int, int),
        void (*)({0}*)",
            o.name
        )?;
    }
    if o.object_type == ObjectType::Tree {
        write!(
            w,
            ",
        void (*)(const {0}*, option_quintptr),
        void (*)({0}*),
        void (*)({0}*),
        void (*)({0}*, quintptr, quintptr),
        void (*)({0}*),
        void (*)({0}*),
        void (*)({0}*, option_quintptr, int, int),
        void (*)({0}*),
        void (*)({0}*, option_quintptr, int, int, option_quintptr, int),
        void (*)({0}*),
        void (*)({0}*, option_quintptr, int, int),
        void (*)({0}*)",
            o.name
        )?;
    }
    Ok(())
}

fn changed_f(o: &Object, p_name: &str) -> String {
    lower_initial(&o.name) + &upper_initial(p_name) + "Changed"
}

fn constructor_args(w: &mut Vec<u8>, prefix: &str, o: &Object, conf: &Config) -> Result<()> {
    let lcname = snake_case(&o.name);
    for (name, p) in &o.properties {
        if let Type::Object(object) = &p.property_type {
            write!(w, ", {}m_{}", prefix, name)?;
            constructor_args(w, &format!("m_{}->", name), object, conf)?;
        } else {
            write!(w, ",\n        {}", changed_f(o, name))?;
        }
    }
    if o.object_type == ObjectType::List {
        writeln!(
            w,
            ",
        [](const {0}* o) {{
            Q_EMIT o->newDataReady(QModelIndex());
        }},
        []({0}* o) {{
            Q_EMIT o->layoutAboutToBeChanged();
        }},
        []({0}* o) {{
            o->updatePersistentIndexes();
            Q_EMIT o->layoutChanged();
        }},
        []({0}* o, quintptr first, quintptr last) {{
            o->dataChanged(o->createIndex(first, 0, first),
                       o->createIndex(last, {1}, last));
        }},
        []({0}* o) {{
            o->beginResetModel();
        }},
        []({0}* o) {{
            o->endResetModel();
        }},
        []({0}* o, int first, int last) {{
            o->beginInsertRows(QModelIndex(), first, last);
        }},
        []({0}* o) {{
            o->endInsertRows();
        }},
        []({0}* o, int first, int last, int destination) {{
            o->beginMoveRows(QModelIndex(), first, last, QModelIndex(), destination);
        }},
        []({0}* o) {{
            o->endMoveRows();
        }},
        []({0}* o, int first, int last) {{
            o->beginRemoveRows(QModelIndex(), first, last);
        }},
        []({0}* o) {{
            o->endRemoveRows();
        }}",
            o.name,
            o.column_count() - 1
        )?;
    }
    if o.object_type == ObjectType::Tree {
        writeln!(
            w,
            ",
        [](const {0}* o, option_quintptr id) {{
            if (id.some) {{
                int row = {1}_row(o->m_d, id.value);
                Q_EMIT o->newDataReady(o->createIndex(row, 0, id.value));
            }} else {{
                Q_EMIT o->newDataReady(QModelIndex());
            }}
        }},
        []({0}* o) {{
            Q_EMIT o->layoutAboutToBeChanged();
        }},
        []({0}* o) {{
            o->updatePersistentIndexes();
            Q_EMIT o->layoutChanged();
        }},
        []({0}* o, quintptr first, quintptr last) {{
            quintptr frow = {1}_row(o->m_d, first);
            quintptr lrow = {1}_row(o->m_d, first);
            o->dataChanged(o->createIndex(frow, 0, first),
                       o->createIndex(lrow, {2}, last));
        }},
        []({0}* o) {{
            o->beginResetModel();
        }},
        []({0}* o) {{
            o->endResetModel();
        }},
        []({0}* o, option_quintptr id, int first, int last) {{
            if (id.some) {{
                int row = {1}_row(o->m_d, id.value);
                o->beginInsertRows(o->createIndex(row, 0, id.value), first, last);
            }} else {{
                o->beginInsertRows(QModelIndex(), first, last);
            }}
        }},
        []({0}* o) {{
            o->endInsertRows();
        }},
        []({0}* o, option_quintptr sourceParent, int first, int last, option_quintptr destinationParent, int destination) {{
            QModelIndex s;
            if (sourceParent.some) {{
                int row = {1}_row(o->m_d, sourceParent.value);
                s = o->createIndex(row, 0, sourceParent.value);
            }}
            QModelIndex d;
            if (destinationParent.some) {{
                int row = {1}_row(o->m_d, destinationParent.value);
                d = o->createIndex(row, 0, destinationParent.value);
            }}
            o->beginMoveRows(s, first, last, d, destination);
        }},
        []({0}* o) {{
            o->endMoveRows();
        }},
        []({0}* o, option_quintptr id, int first, int last) {{
            if (id.some) {{
                int row = {1}_row(o->m_d, id.value);
                o->beginRemoveRows(o->createIndex(row, 0, id.value), first, last);
            }} else {{
                o->beginRemoveRows(QModelIndex(), first, last);
            }}
        }},
        []({0}* o) {{
            o->endRemoveRows();
        }}",
            o.name,
            lcname,
            o.column_count() - 1
        )?;
    }
    Ok(())
}

pub fn write_header(conf: &Config) -> Result<()> {
    let mut h_file = conf.config_file.parent().unwrap().join(&conf.cpp_file);
    h_file.set_extension("h");
    let mut h = Vec::new();
    let guard = h_file
        .file_name()
        .unwrap()
        .to_string_lossy()
        .replace('.', "_")
        .to_uppercase();
    writeln!(
        h,
        "/* generated by rust_qt_binding_generator */
#ifndef {0}
#define {0}

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
",
        guard
    )?;

    for name in conf.objects.keys() {
        writeln!(h, "class {};", name)?;
    }
    for object in conf.objects.values() {
        write_header_object(&mut h, object, conf)?;
    }
    writeln!(h, "#endif // {}", guard)?;

    write_if_different(h_file, &h)?;
    Ok(())
}

pub fn write_cpp(conf: &Config) -> Result<()> {
    let mut w = Vec::new();
    let mut h_file = conf.config_file.parent().unwrap().join(&conf.cpp_file);
    h_file.set_extension("h");
    let file_name = h_file.file_name().unwrap().to_string_lossy();
    writeln!(
        w,
        "/* generated by rust_qt_binding_generator */
#include \"{}\"

namespace {{",
        file_name
    )?;
    for option in conf.optional_types() {
        if option != "QString" && option != "QByteArray" {
            writeln!(
                w,
                "
    struct option_{} {{
    public:
        {0} value;
        bool some;
        operator QVariant() const {{
            if (some) {{
                return QVariant::fromValue(value);
            }}
            return QVariant();
        }}
    }};
    static_assert(std::is_pod<option_{0}>::value, \"option_{0} must be a POD type.\");",
                option
            )?;
        }
    }
    if conf.types().contains("QString") {
        writeln!(
            w,
            "
    typedef void (*qstring_set)(QString* val, const char* utf8, int nbytes);
    void set_qstring(QString* val, const char* utf8, int nbytes) {{
        *val = QString::fromUtf8(utf8, nbytes);
    }}"
        )?;
    }
    if conf.types().contains("QByteArray") {
        writeln!(
            w,
            "
    typedef void (*qbytearray_set)(QByteArray* val, const char* bytes, int nbytes);
    void set_qbytearray(QByteArray* v, const char* bytes, int nbytes) {{
        if (v->isNull() && nbytes == 0) {{
            *v = QByteArray(bytes, nbytes);
        }} else {{
            v->truncate(0);
            v->append(bytes, nbytes);
        }}
    }}"
        )?;
    }
    if conf.has_list_or_tree() {
        writeln!(
            w,
            "
    struct qmodelindex_t {{
        int row;
        quintptr id;
    }};
    inline QVariant cleanNullQVariant(const QVariant& v) {{
        return (v.isNull()) ?QVariant() :v;
    }}"
        )?;
    }
    for (name, o) in &conf.objects {
        for (p_name, p) in &o.properties {
            if p.is_object() {
                continue;
            }
            writeln!(w, "    inline void {}({}* o)", changed_f(o, p_name), name)?;
            writeln!(w, "    {{\n        Q_EMIT o->{}Changed();\n    }}", p_name)?;
        }
    }
    writeln!(w, "}}")?;

    for o in conf.objects.values() {
        if o.object_type != ObjectType::Object {
            write_cpp_model(&mut w, o)?;
        }
        writeln!(w, "extern \"C\" {{")?;
        write_object_c_decl(&mut w, o, conf)?;
        writeln!(w, "}};\n")?;
    }

    for o in conf.objects.values() {
        write_cpp_object(&mut w, o, conf)?;
    }

    writeln!(w, "extern \"C\" {{
    void qmetaobject__invokeMethod__0(QObject *obj, const char *member) {{
        QMetaObject::invokeMethod(obj, member);
    }}
}}")?;


    let file = conf.config_file.parent().unwrap().join(&conf.cpp_file);
    write_if_different(file, &w)
}
