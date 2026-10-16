#include "OD/pch.h"
#include "Serializer.h"

#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <istream>
#include <ostream>
#include <utility>
#include <limits>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <cstring>
#include <cerrno>

namespace OD{

/*namespace {
enum class Kind : std::uint8_t { Object, Array, Bool, Signed, Unsigned, Floating, String };
struct Node {
    std::string name;
    std::uint8_t kind = static_cast<std::uint8_t>(Kind::Object);
    bool boolean = false;
    std::int64_t signed_number = 0;
    std::uint64_t unsigned_number = 0;
    double floating_number = 0;
    std::string text;
    std::vector<Node> children;
    template<class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(name), CEREAL_NVP(kind), CEREAL_NVP(boolean),
           CEREAL_NVP(signed_number), CEREAL_NVP(unsigned_number),
           CEREAL_NVP(floating_number), CEREAL_NVP(text), CEREAL_NVP(children));
    }
};
struct Frame { Node* node; std::size_t next = 0; };

void check_kind(const Node& node, Kind expected) {
    if (node.kind != static_cast<std::uint8_t>(expected) &&
        !((expected == Kind::Signed || expected == Kind::Unsigned) &&
          (node.kind == static_cast<std::uint8_t>(Kind::Signed) || node.kind == static_cast<std::uint8_t>(Kind::Unsigned))))
        throw std::runtime_error("Serialized field has an unexpected type: " + node.name);
}
}
namespace {
void write_escaped(std::ostream& out, const std::string& s) {
    out << '"';
    const char* hex = "0123456789abcdef";
    for (unsigned char c : s) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        default:
            if (c < 0x20) { out << "\\u00" << hex[c >> 4] << hex[c & 15]; }
            else out << static_cast<char>(c);
        }
    }
    out << '"';
}
void write_json_node(std::ostream& out, const Node& n, int depth) {
    auto indent = [&](int d) { for (int i = 0; i < d * 2; ++i) out.put(' '); };
    switch (static_cast<Kind>(n.kind)) {
    case Kind::Object:
    case Kind::Array: {
        bool object = static_cast<Kind>(n.kind) == Kind::Object;
        out << (object ? '{' : '[');
        for (std::size_t i = 0; i < n.children.size(); ++i) {
            out << '\n'; indent(depth + 1);
            const Node& child = n.children[i];
            if (object) { write_escaped(out, child.name); out << ": "; }
            write_json_node(out, child, depth + 1);
            if (i + 1 < n.children.size()) out << ',';
        }
        if (!n.children.empty()) { out << '\n'; indent(depth); }
        out << (object ? '}' : ']');
        return;
    }
    case Kind::Bool: out << (n.boolean ? "true" : "false"); return;
    case Kind::Signed: out << n.signed_number; return;
    case Kind::Unsigned: out << n.unsigned_number; return;
    case Kind::Floating: {
        if (!std::isfinite(n.floating_number)) throw std::runtime_error("JSON cannot represent non-finite floats");
        std::ostringstream temp;
        temp << std::setprecision(std::numeric_limits<double>::max_digits10) << n.floating_number;
        std::string num = temp.str();
        if (num.find_first_of(".eE") == std::string::npos) num += ".0";
        out << num;
        return;
    }
    case Kind::String: write_escaped(out, n.text); return;
    }
    throw std::runtime_error("Invalid node kind");
}
class JsonParser {
    const std::string data;
    std::size_t p = 0;
    void ws() { while (p < data.size() && (data[p]==' ' || data[p]=='\n' || data[p]=='\r' || data[p]=='\t')) ++p; }
    char peek() { ws(); return p < data.size() ? data[p] : '\0'; }
    bool eat(char c) { if (peek() == c) { ++p; return true; } return false; }
    void require(char c) { if (!eat(c)) throw std::runtime_error("Invalid JSON: expected delimiter at offset " + std::to_string(p)); }
    static int digit(char c) { if (c>='0' && c<='9') return c-'0'; if (c>='a'&&c<='f') return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }
    unsigned codepoint() {
        if (p+4 > data.size()) throw std::runtime_error("Invalid JSON unicode escape");
        unsigned cp=0;
        for (int i=0;i<4;++i) { int d=digit(data[p++]); if (d<0) throw std::runtime_error("Invalid JSON unicode escape"); cp=(cp<<4)|static_cast<unsigned>(d); }
        return cp;
    }
    static void utf8(std::string& s, unsigned cp) {
        if(cp<=0x7F) s+=static_cast<char>(cp);
        else if(cp<=0x7FF) {s+=static_cast<char>(0xC0|(cp>>6));s+=static_cast<char>(0x80|(cp&63));}
        else if(cp<=0xFFFF) {s+=static_cast<char>(0xE0|(cp>>12));s+=static_cast<char>(0x80|((cp>>6)&63));s+=static_cast<char>(0x80|(cp&63));}
        else {s+=static_cast<char>(0xF0|(cp>>18));s+=static_cast<char>(0x80|((cp>>12)&63));s+=static_cast<char>(0x80|((cp>>6)&63));s+=static_cast<char>(0x80|(cp&63));}
    }
    std::string string() {
        require('"'); std::string s;
        while(p<data.size()) {
            unsigned char c=static_cast<unsigned char>(data[p++]);
            if(c=='"') return s;
            if(c<0x20) throw std::runtime_error("Unescaped control character in JSON string");
            if(c!='\\') { s+=static_cast<char>(c); continue; }
            if(p>=data.size()) break;
            char e=data[p++];
            switch(e) {
            case '"': s+='"';break; case '\\':s+='\\';break;case '/':s+='/';break;
            case 'n':s+='\n';break;case 'r':s+='\r';break;case 't':s+='\t';break;case 'b':s+='\b';break;case 'f':s+='\f';break;
            case 'u': {
                unsigned cp=codepoint();
                if(cp>=0xD800 && cp<=0xDBFF) { if(p+2>data.size()||data[p++]!='\\'||data[p++]!='u')throw std::runtime_error("Missing low surrogate"); unsigned lo=codepoint(); if(lo<0xDC00||lo>0xDFFF)throw std::runtime_error("Invalid low surrogate");cp=0x10000+((cp-0xD800)<<10)+(lo-0xDC00); }
                else if(cp>=0xDC00 && cp<=0xDFFF) throw std::runtime_error("Lone low surrogate");
                utf8(s, cp);break;
            }
            default:throw std::runtime_error("Invalid JSON escape");
            }
        }
        throw std::runtime_error("Unterminated JSON string");
    }
    Node parse(std::string name="") {
        Node n; n.name=std::move(name);
        char c=peek();
        if(c=='{') {
            n.kind=static_cast<std::uint8_t>(Kind::Object);++p;
            if(eat('}'))return n;
            do {std::string key=string(); require(':');n.children.push_back(parse(std::move(key)));} while(eat(','));
            require('}');return n;
        }
        if(c=='[') {
            n.kind=static_cast<std::uint8_t>(Kind::Array);++p;
            if(eat(']'))return n;
            do {n.children.push_back(parse());}while(eat(','));
            require(']');return n;
        }
        if(c=='"'){n.kind=static_cast<std::uint8_t>(Kind::String);n.text=string();return n;}
        if(data.compare(p,4,"true")==0){p+=4;n.kind=static_cast<std::uint8_t>(Kind::Bool);n.boolean=true;return n;}
        if(data.compare(p,5,"false")==0){p+=5;n.kind=static_cast<std::uint8_t>(Kind::Bool);return n;}
        if(c=='-'||(c>='0'&&c<='9')) {
            std::size_t start=p;
            if(data[p]=='-')++p;
            if(p>=data.size())throw std::runtime_error("Invalid JSON number");
            if(data[p]=='0')++p;
            else {if(data[p]<'1'||data[p]>'9')throw std::runtime_error("Invalid JSON number");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
            bool floating=false;
            if(p<data.size()&&data[p]=='.'){floating=true;++p;if(p==data.size()||data[p]<'0'||data[p]>'9')throw std::runtime_error("Invalid fraction");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
            if(p<data.size()&&(data[p]=='e'||data[p]=='E')){floating=true;++p;if(p<data.size()&&(data[p]=='+'||data[p]=='-'))++p;if(p==data.size()||data[p]<'0'||data[p]>'9')throw std::runtime_error("Invalid exponent");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
            std::string tok=data.substr(start,p-start);
            if(floating){n.kind=static_cast<std::uint8_t>(Kind::Floating); char* end=nullptr; errno=0;n.floating_number=std::strtod(tok.c_str(),&end);if(errno==ERANGE||end!=tok.c_str()+tok.size()||!std::isfinite(n.floating_number))throw std::runtime_error("Invalid JSON floating number");}
            else if(tok[0]=='-'){n.kind=static_cast<std::uint8_t>(Kind::Signed);auto res=std::from_chars(tok.data(),tok.data()+tok.size(),n.signed_number);if(res.ec!=std::errc{})throw std::runtime_error("Signed integer overflow");}
            else {n.kind=static_cast<std::uint8_t>(Kind::Unsigned);auto res=std::from_chars(tok.data(),tok.data()+tok.size(),n.unsigned_number);if(res.ec!=std::errc{})throw std::runtime_error("Unsigned integer overflow");}
            return n;
        }
        throw std::runtime_error("Invalid JSON value at offset " + std::to_string(p));
    }
public:
    explicit JsonParser(std::istream& stream) : data(std::istreambuf_iterator<char>(stream), {}) {}
    Node root() { Node n=parse();if(peek()!='\0')throw std::runtime_error("Trailing characters after JSON");return n; }
};
} // anonymous namespace

struct Serializer::Impl {
    explicit Impl(Mode m) : mode(m) { stack.push_back({&root, 0}); }
    Mode mode;
    Node root;
    std::vector<Frame> stack;

    Node& fetch(const char* name, Kind kind) {
        auto& frame = stack.back();
        Node* parent = frame.node;
        if (mode == Mode::Write) {
            parent->children.emplace_back();
            Node& result = parent->children.back();
            result.name = name;
            result.kind = static_cast<std::uint8_t>(kind);
            return result;
        }
        Node* result = nullptr;
        if (parent->kind == static_cast<std::uint8_t>(Kind::Array)) {
            if (frame.next >= parent->children.size()) throw std::runtime_error("Array exhausted");
            result = &parent->children[frame.next++];
        } else {
            // Named lookup makes reading independent of field order.
            for (Node& child : parent->children) {
                if (child.name == name) { result = &child; break; }
            }
            if (!result) throw std::runtime_error(std::string("Missing field: ") + name);
        }
        check_kind(*result, kind);
        return *result;
    }
};
Serializer::Serializer(Mode mode) : impl_(std::make_unique<Impl>(mode)) {}
Serializer::~Serializer() = default;
Serializer::Serializer(Serializer&&) noexcept = default;
Serializer& Serializer::operator=(Serializer&&) noexcept = default;
bool Serializer::IsReading() const noexcept { return impl_->mode == Mode::Read; }
void Serializer::BeginObject(const char* name) {
    Node& node = impl_->fetch(name, Kind::Object);
    impl_->stack.push_back({&node, 0});
}
std::size_t Serializer::BeginArray(const char* name, std::size_t count) {
    Node& node = impl_->fetch(name, Kind::Array);
    impl_->stack.push_back({&node, 0});
    return IsReading() ? node.children.size() : count;
}
void Serializer::End() noexcept { impl_->stack.pop_back(); }

void Serializer::Field(const char* name, bool& value) {
    Node& n = impl_->fetch(name, Kind::Bool);
    if (IsWriting()) n.boolean = value; else value = n.boolean;
}
void Serializer::Field(const char* name, std::string& value) {
    Node& n = impl_->fetch(name, Kind::String);
    if (IsWriting()) n.text = value; else value = n.text;
}
void Serializer::Field(const char* name, float& value) {
    Node& n = impl_->fetch(name, Kind::Floating);
    if (IsWriting()) n.floating_number = value; else value = static_cast<float>(n.floating_number);
}
void Serializer::Field(const char* name, double& value) {
    Node& n = impl_->fetch(name, Kind::Floating);
    if (IsWriting()) n.floating_number = value; else value = n.floating_number;
}
#define SIGNED_FIELD(Type) \
void Serializer::Field(const char* name, Type& value) { \
    Node& n = impl_->fetch(name, Kind::Signed); \
    if (IsWriting()) n.signed_number = value; \
    else { \
        if ((n.kind == static_cast<std::uint8_t>(Kind::Unsigned) && n.unsigned_number > static_cast<std::uint64_t>(std::numeric_limits<Type>::max())) || \
            (n.kind == static_cast<std::uint8_t>(Kind::Signed) && (n.signed_number < static_cast<std::int64_t>(std::numeric_limits<Type>::min()) || n.signed_number > static_cast<std::int64_t>(std::numeric_limits<Type>::max())))) \
            throw std::runtime_error("Signed integer out of range"); \
        value = n.kind == static_cast<std::uint8_t>(Kind::Unsigned) ? static_cast<Type>(n.unsigned_number) : static_cast<Type>(n.signed_number); \
    } \
}
#define UNSIGNED_FIELD(Type) \
void Serializer::Field(const char* name, Type& value) { \
    Node& n = impl_->fetch(name, Kind::Unsigned); \
    if (IsWriting()) n.unsigned_number = value; \
    else { \
        if ((n.kind == static_cast<std::uint8_t>(Kind::Signed) && (n.signed_number < 0 || static_cast<std::uint64_t>(n.signed_number) > static_cast<std::uint64_t>(std::numeric_limits<Type>::max()))) || \
            (n.kind == static_cast<std::uint8_t>(Kind::Unsigned) && n.unsigned_number > static_cast<std::uint64_t>(std::numeric_limits<Type>::max()))) \
            throw std::runtime_error("Unsigned integer out of range"); \
        value = n.kind == static_cast<std::uint8_t>(Kind::Signed) ? static_cast<Type>(n.signed_number) : static_cast<Type>(n.unsigned_number); \
    } \
}
SIGNED_FIELD(std::int8_t)
SIGNED_FIELD(std::int16_t)
SIGNED_FIELD(std::int32_t)
SIGNED_FIELD(std::int64_t)
UNSIGNED_FIELD(std::uint8_t)
UNSIGNED_FIELD(std::uint16_t)
UNSIGNED_FIELD(std::uint32_t)
UNSIGNED_FIELD(std::uint64_t)
#undef SIGNED_FIELD
#undef UNSIGNED_FIELD
void Serializer::Save(std::ostream& stream, Format format) const {
    if (IsReading()) throw std::logic_error("Cannot save in read mode");
    if (impl_->stack.size() != 1) throw std::logic_error("Unclosed serialization scope");
    if (format == Format::Json) {
        write_json_node(stream, impl_->root, 0);
        stream << '\n';
        if (!stream) throw std::runtime_error("JSON write failed");
    } else {
        cereal::BinaryOutputArchive archive(stream);
        archive(impl_->root);
    }
}
void Serializer::Load(std::istream& stream, Format format) {
    if (IsWriting()) throw std::logic_error("Cannot load in write mode");
    if (format == Format::Json) {
        impl_->root = JsonParser(stream).root();
    } else {
        cereal::BinaryInputArchive archive(stream);
        archive(impl_->root);
    }
    check_kind(impl_->root, Kind::Object);
    impl_->stack.clear();
    impl_->stack.push_back({&impl_->root, 0});
}*/

namespace {
    enum class Kind : std::uint8_t { Object, Array, Bool, Signed, Unsigned, Floating, String };
    
    struct Node {
        std::string name;
        std::uint8_t kind = static_cast<std::uint8_t>(Kind::Object);
        std::uint8_t width = 8; // binary scalar byte width (1/2/4/8)
        bool boolean = false;
        std::int64_t signed_number = 0;
        std::uint64_t unsigned_number = 0;
        double floating_number = 0;
        std::string text;
        std::vector<Node> children;
        template<class Archive>
        void serialize(Archive& ar) {
            ar(CEREAL_NVP(name), CEREAL_NVP(kind), CEREAL_NVP(boolean),
            CEREAL_NVP(signed_number), CEREAL_NVP(unsigned_number),
            CEREAL_NVP(floating_number), CEREAL_NVP(text), CEREAL_NVP(children));
        }
    };
    
    struct Frame { Node* node; std::size_t next = 0; };

    void check_kind(const Node& node, Kind expected) {
        if (node.kind != static_cast<std::uint8_t>(expected) &&
            !((expected == Kind::Signed || expected == Kind::Unsigned) &&
            (node.kind == static_cast<std::uint8_t>(Kind::Signed) || node.kind == static_cast<std::uint8_t>(Kind::Unsigned))))
            throw std::runtime_error("Serialized field has an unexpected type: " + node.name);
    }
}

namespace {
    void write_escaped(std::ostream& out, const std::string& s) {
        out << '"';
        const char* hex = "0123456789abcdef";
        for (unsigned char c : s) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            default:
                if (c < 0x20) { out << "\\u00" << hex[c >> 4] << hex[c & 15]; }
                else out << static_cast<char>(c);
            }
        }
        out << '"';
    }
    
    void write_json_node(std::ostream& out, const Node& n, int depth) {
        auto indent = [&](int d) { for (int i = 0; i < d * 2; ++i) out.put(' '); };
        switch (static_cast<Kind>(n.kind)) {
        case Kind::Object:
        case Kind::Array: {
            bool object = static_cast<Kind>(n.kind) == Kind::Object;
            out << (object ? '{' : '[');
            for (std::size_t i = 0; i < n.children.size(); ++i) {
                out << '\n'; indent(depth + 1);
                const Node& child = n.children[i];
                if (object) { write_escaped(out, child.name); out << ": "; }
                write_json_node(out, child, depth + 1);
                if (i + 1 < n.children.size()) out << ',';
            }
            if (!n.children.empty()) { out << '\n'; indent(depth); }
            out << (object ? '}' : ']');
            return;
        }
        case Kind::Bool: out << (n.boolean ? "true" : "false"); return;
        case Kind::Signed: out << n.signed_number; return;
        case Kind::Unsigned: out << n.unsigned_number; return;
        case Kind::Floating: {
            if (!std::isfinite(n.floating_number)) throw std::runtime_error("JSON cannot represent non-finite floats");
            std::ostringstream temp;
            temp << std::setprecision(std::numeric_limits<double>::max_digits10) << n.floating_number;
            std::string num = temp.str();
            if (num.find_first_of(".eE") == std::string::npos) num += ".0";
            out << num;
            return;
        }
        case Kind::String: write_escaped(out, n.text); return;
        }
        throw std::runtime_error("Invalid node kind");
    }
    
    class JsonParser {
        const std::string data;
        std::size_t p = 0;
        void ws() { while (p < data.size() && (data[p]==' ' || data[p]=='\n' || data[p]=='\r' || data[p]=='\t')) ++p; }
        char peek() { ws(); return p < data.size() ? data[p] : '\0'; }
        bool eat(char c) { if (peek() == c) { ++p; return true; } return false; }
        void require(char c) { if (!eat(c)) throw std::runtime_error("Invalid JSON: expected delimiter at offset " + std::to_string(p)); }
        static int digit(char c) { if (c>='0' && c<='9') return c-'0'; if (c>='a'&&c<='f') return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }
        unsigned codepoint() {
            if (p+4 > data.size()) throw std::runtime_error("Invalid JSON unicode escape");
            unsigned cp=0;
            for (int i=0;i<4;++i) { int d=digit(data[p++]); if (d<0) throw std::runtime_error("Invalid JSON unicode escape"); cp=(cp<<4)|static_cast<unsigned>(d); }
            return cp;
        }
        static void utf8(std::string& s, unsigned cp) {
            if(cp<=0x7F) s+=static_cast<char>(cp);
            else if(cp<=0x7FF) {s+=static_cast<char>(0xC0|(cp>>6));s+=static_cast<char>(0x80|(cp&63));}
            else if(cp<=0xFFFF) {s+=static_cast<char>(0xE0|(cp>>12));s+=static_cast<char>(0x80|((cp>>6)&63));s+=static_cast<char>(0x80|(cp&63));}
            else {s+=static_cast<char>(0xF0|(cp>>18));s+=static_cast<char>(0x80|((cp>>12)&63));s+=static_cast<char>(0x80|((cp>>6)&63));s+=static_cast<char>(0x80|(cp&63));}
        }
        std::string string() {
            require('"'); std::string s;
            while(p<data.size()) {
                unsigned char c=static_cast<unsigned char>(data[p++]);
                if(c=='"') return s;
                if(c<0x20) throw std::runtime_error("Unescaped control character in JSON string");
                if(c!='\\') { s+=static_cast<char>(c); continue; }
                if(p>=data.size()) break;
                char e=data[p++];
                switch(e) {
                case '"': s+='"';break; case '\\':s+='\\';break;case '/':s+='/';break;
                case 'n':s+='\n';break;case 'r':s+='\r';break;case 't':s+='\t';break;case 'b':s+='\b';break;case 'f':s+='\f';break;
                case 'u': {
                    unsigned cp=codepoint();
                    if(cp>=0xD800 && cp<=0xDBFF) { if(p+2>data.size()||data[p++]!='\\'||data[p++]!='u')throw std::runtime_error("Missing low surrogate"); unsigned lo=codepoint(); if(lo<0xDC00||lo>0xDFFF)throw std::runtime_error("Invalid low surrogate");cp=0x10000+((cp-0xD800)<<10)+(lo-0xDC00); }
                    else if(cp>=0xDC00 && cp<=0xDFFF) throw std::runtime_error("Lone low surrogate");
                    utf8(s, cp);break;
                }
                default:throw std::runtime_error("Invalid JSON escape");
                }
            }
            throw std::runtime_error("Unterminated JSON string");
        }
        Node parse(std::string name="") {
            Node n; n.name=std::move(name);
            char c=peek();
            if(c=='{') {
                n.kind=static_cast<std::uint8_t>(Kind::Object);++p;
                if(eat('}'))return n;
                do {std::string key=string(); require(':');n.children.push_back(parse(std::move(key)));} while(eat(','));
                require('}');return n;
            }
            if(c=='[') {
                n.kind=static_cast<std::uint8_t>(Kind::Array);++p;
                if(eat(']'))return n;
                do {n.children.push_back(parse());}while(eat(','));
                require(']');return n;
            }
            if(c=='"'){n.kind=static_cast<std::uint8_t>(Kind::String);n.text=string();return n;}
            if(data.compare(p,4,"true")==0){p+=4;n.kind=static_cast<std::uint8_t>(Kind::Bool);n.boolean=true;return n;}
            if(data.compare(p,5,"false")==0){p+=5;n.kind=static_cast<std::uint8_t>(Kind::Bool);return n;}
            if(c=='-'||(c>='0'&&c<='9')) {
                std::size_t start=p;
                if(data[p]=='-')++p;
                if(p>=data.size())throw std::runtime_error("Invalid JSON number");
                if(data[p]=='0')++p;
                else {if(data[p]<'1'||data[p]>'9')throw std::runtime_error("Invalid JSON number");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
                bool floating=false;
                if(p<data.size()&&data[p]=='.'){floating=true;++p;if(p==data.size()||data[p]<'0'||data[p]>'9')throw std::runtime_error("Invalid fraction");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
                if(p<data.size()&&(data[p]=='e'||data[p]=='E')){floating=true;++p;if(p<data.size()&&(data[p]=='+'||data[p]=='-'))++p;if(p==data.size()||data[p]<'0'||data[p]>'9')throw std::runtime_error("Invalid exponent");while(p<data.size()&&data[p]>='0'&&data[p]<='9')++p;}
                std::string tok=data.substr(start,p-start);
                if(floating){n.kind=static_cast<std::uint8_t>(Kind::Floating); char* end=nullptr; errno=0;n.floating_number=std::strtod(tok.c_str(),&end);if(errno==ERANGE||end!=tok.c_str()+tok.size()||!std::isfinite(n.floating_number))throw std::runtime_error("Invalid JSON floating number");}
                else if(tok[0]=='-'){n.kind=static_cast<std::uint8_t>(Kind::Signed);auto res=std::from_chars(tok.data(),tok.data()+tok.size(),n.signed_number);if(res.ec!=std::errc{})throw std::runtime_error("Signed integer overflow");}
                else {n.kind=static_cast<std::uint8_t>(Kind::Unsigned);auto res=std::from_chars(tok.data(),tok.data()+tok.size(),n.unsigned_number);if(res.ec!=std::errc{})throw std::runtime_error("Unsigned integer overflow");}
                return n;
            }
            throw std::runtime_error("Invalid JSON value at offset " + std::to_string(p));
        }
    public:
        explicit JsonParser(std::istream& stream) : data(std::istreambuf_iterator<char>(stream), {}) {}
        Node root() { Node n=parse();if(peek()!='\0')throw std::runtime_error("Trailing characters after JSON");return n; }
    };
} // anonymous namespace

struct Serializer::Impl {
    explicit Impl(Mode m) : mode(m) { stack.push_back({&root, 0}); }
    Mode mode;
    Node root;
    std::vector<Frame> stack;
    std::unique_ptr<cereal::BinaryInputArchive> binary_reader;
    std::istream* binary_stream = nullptr;

    Node& fetch(const char* name, Kind kind) {
        auto& frame = stack.back();
        Node* parent = frame.node;
        if (mode == Mode::Write) {
            parent->children.emplace_back();
            Node& result = parent->children.back();
            result.name = name;
            result.kind = static_cast<std::uint8_t>(kind);
            return result;
        }
        Node* result = nullptr;
        if (parent->kind == static_cast<std::uint8_t>(Kind::Array)) {
            if (frame.next >= parent->children.size()) throw std::runtime_error("Array exhausted");
            result = &parent->children[frame.next++];
        } else {
            // Named lookup makes reading independent of field order.
            for (Node& child : parent->children) {
                if (child.name == name) { result = &child; break; }
            }
            if (!result) throw std::runtime_error(std::string("Missing field: ") + name);
        }
        check_kind(*result, kind);
        return *result;
    }
};

// Compact, schema-ordered binary: NO field names, node tags, or unused Node members.
// Arrays and strings have cereal's uint64 length prefix; objects contain fields in order.
void write_binary_node(cereal::BinaryOutputArchive& ar, const Node& node) {
    switch (static_cast<Kind>(node.kind)) {
    case Kind::Object:
        for (const Node& child : node.children) write_binary_node(ar, child);
        return;
    case Kind::Array: {
        const auto count = static_cast<std::uint64_t>(node.children.size());
        ar(count);
        for (const Node& child : node.children) write_binary_node(ar, child);
        return;
    }
    case Kind::Bool: ar(node.boolean); return;
    case Kind::String: ar(node.text); return;
    case Kind::Floating:
        if (node.width == 4) { float v = static_cast<float>(node.floating_number); ar(v); }
        else { double v = node.floating_number; ar(v); }
        return;
    case Kind::Signed:
        switch (node.width) {
        case 1: { auto v = static_cast<std::int8_t>(node.signed_number); ar(v); return; }
        case 2: { auto v = static_cast<std::int16_t>(node.signed_number); ar(v); return; }
        case 4: { auto v = static_cast<std::int32_t>(node.signed_number); ar(v); return; }
        case 8: { auto v = node.signed_number; ar(v); return; }
        default: throw std::runtime_error("Invalid binary integer width");
        }
    case Kind::Unsigned:
        switch (node.width) {
        case 1: { auto v = static_cast<std::uint8_t>(node.unsigned_number); ar(v); return; }
        case 2: { auto v = static_cast<std::uint16_t>(node.unsigned_number); ar(v); return; }
        case 4: { auto v = static_cast<std::uint32_t>(node.unsigned_number); ar(v); return; }
        case 8: { auto v = node.unsigned_number; ar(v); return; }
        default: throw std::runtime_error("Invalid binary integer width");
        }
    }
    throw std::runtime_error("Invalid binary node kind");
}

Serializer::Serializer(Mode mode) : impl_(std::make_unique<Impl>(mode)) {}

Serializer::~Serializer() = default;

Serializer::Serializer(Serializer&&) noexcept = default;

Serializer& Serializer::operator=(Serializer&&) noexcept = default;

bool Serializer::IsReading() const noexcept { return impl_->mode == Mode::Read; }

void Serializer::BeginObject(const char* name) {
    if (impl_->binary_reader) return; // schema-ordered binary has no object delimiters
    Node& node = impl_->fetch(name, Kind::Object);
    impl_->stack.push_back({&node, 0});
}

std::size_t Serializer::BeginArray(const char* name, std::size_t count) {
    if (impl_->binary_reader) {
        std::uint64_t size = 0;
        (*impl_->binary_reader)(size);
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("Binary array size overflow");
        return static_cast<std::size_t>(size);
    }
    Node& node = impl_->fetch(name, Kind::Array);
    impl_->stack.push_back({&node, 0});
    return IsReading() ? node.children.size() : count;
}

void Serializer::End() noexcept { if (!impl_->binary_reader) impl_->stack.pop_back(); }

void Serializer::Field(const char* name, bool& value) {
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; }
    Node& n = impl_->fetch(name, Kind::Bool);
    if (IsWriting()) n.boolean = value; else value = n.boolean;
}

void Serializer::Field(const char* name, std::string& value) {
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; }
    Node& n = impl_->fetch(name, Kind::String);
    if (IsWriting()) n.text = value; else value = n.text;
}

void Serializer::Field(const char* name, float& value) {
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; }
    Node& n = impl_->fetch(name, Kind::Floating);
    if (IsWriting()) { n.width = sizeof(value); n.floating_number = value; } else value = static_cast<float>(n.floating_number);
}

void Serializer::Field(const char* name, double& value) {
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; }
    Node& n = impl_->fetch(name, Kind::Floating);
    if (IsWriting()) { n.width = sizeof(value); n.floating_number = value; } else value = n.floating_number;
}

#define SIGNED_FIELD(Type) \
void Serializer::Field(const char* name, Type& value) { \
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; } \
    Node& n = impl_->fetch(name, Kind::Signed); \
    if (IsWriting()) { n.width = sizeof(value); n.signed_number = value; } \
    else { \
        if ((n.kind == static_cast<std::uint8_t>(Kind::Unsigned) && n.unsigned_number > static_cast<std::uint64_t>(std::numeric_limits<Type>::max())) || \
            (n.kind == static_cast<std::uint8_t>(Kind::Signed) && (n.signed_number < static_cast<std::int64_t>(std::numeric_limits<Type>::min()) || n.signed_number > static_cast<std::int64_t>(std::numeric_limits<Type>::max())))) \
            throw std::runtime_error("Signed integer out of range"); \
        value = n.kind == static_cast<std::uint8_t>(Kind::Unsigned) ? static_cast<Type>(n.unsigned_number) : static_cast<Type>(n.signed_number); \
    } \
}

#define UNSIGNED_FIELD(Type) \
void Serializer::Field(const char* name, Type& value) { \
    if (impl_->binary_reader) { (*impl_->binary_reader)(value); return; } \
    Node& n = impl_->fetch(name, Kind::Unsigned); \
    if (IsWriting()) { n.width = sizeof(value); n.unsigned_number = value; } \
    else { \
        if ((n.kind == static_cast<std::uint8_t>(Kind::Signed) && (n.signed_number < 0 || static_cast<std::uint64_t>(n.signed_number) > static_cast<std::uint64_t>(std::numeric_limits<Type>::max()))) || \
            (n.kind == static_cast<std::uint8_t>(Kind::Unsigned) && n.unsigned_number > static_cast<std::uint64_t>(std::numeric_limits<Type>::max()))) \
            throw std::runtime_error("Unsigned integer out of range"); \
        value = n.kind == static_cast<std::uint8_t>(Kind::Signed) ? static_cast<Type>(n.signed_number) : static_cast<Type>(n.unsigned_number); \
    } \
}

SIGNED_FIELD(std::int8_t)
SIGNED_FIELD(std::int16_t)
SIGNED_FIELD(std::int32_t)
SIGNED_FIELD(std::int64_t)
UNSIGNED_FIELD(std::uint8_t)
UNSIGNED_FIELD(std::uint16_t)
UNSIGNED_FIELD(std::uint32_t)
UNSIGNED_FIELD(std::uint64_t)

#undef SIGNED_FIELD
#undef UNSIGNED_FIELD

void Serializer::Save(std::ostream& stream, Format format) const {
    if (IsReading()) throw std::logic_error("Cannot save in read mode");
    if (impl_->stack.size() != 1) throw std::logic_error("Unclosed serialization scope");
    if (format == Format::Json) {
        write_json_node(stream, impl_->root, 0);
        stream << '\n';
        if (!stream) throw std::runtime_error("JSON write failed");
    } else {
        stream.write("CSB2", 4);
        cereal::BinaryOutputArchive archive(stream);
        write_binary_node(archive, impl_->root);
    }
}

void Serializer::Load(std::istream& stream, Format format) {
    if (IsWriting()) throw std::logic_error("Cannot load in write mode");
    if (format == Format::Json) {
        impl_->root = JsonParser(stream).root();
    } else {
        char magic[4]{};
        stream.read(magic, 4);
        if (!stream || std::memcmp(magic, "CSB2", 4) != 0)
            throw std::runtime_error("Invalid compact binary header (expected CSB2)");
        impl_->binary_stream = &stream;
        impl_->binary_reader = std::make_unique<cereal::BinaryInputArchive>(stream);
    }
    if (!impl_->binary_reader) check_kind(impl_->root, Kind::Object);
    impl_->stack.clear();
    impl_->stack.push_back({&impl_->root, 0});
}

} // namespace compactser
