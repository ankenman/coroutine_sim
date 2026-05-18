#include "csim/tracing/tracer.h"

#include <cassert>

namespace csim::tracing {
namespace {

auto
escape_json_string(const std::string& s) -> std::string
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            }
            else {
                out += c;
            }
        }
    }
    return out;
}

auto
format_ns(time_ps t) -> std::string
{
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << (t / 1000.0) << " ns";
    return s.str();
}

auto
format_hex(uint64_t v) -> std::string
{
    std::ostringstream s;
    s << "0x" << std::hex << v;
    return s.str();
}

} // namespace

auto
Tracer::instance() -> Tracer&
{
    static Tracer instance;
    return instance;
}

Tracer::~Tracer()
{
    close();
}

auto
Tracer::open(const std::string& jsonl_path, const std::string& text_path, sim_t& sim) -> void
{
    assert(sim_ptr == nullptr && "Tracer::open called twice without close");

    sim_ptr = &sim;

    if (!jsonl_path.empty()) {
        jsonl_out.open(jsonl_path);
        assert(jsonl_out.is_open() && "Tracer failed to open JSONL output");
    }

    if (!text_path.empty()) {
        text_out.open(text_path);
        assert(text_out.is_open() && "Tracer failed to open text output");
    }
}

auto
Tracer::close() -> void
{
    if (jsonl_out.is_open()) {
        jsonl_out.flush();
        jsonl_out.close();
    }
    if (text_out.is_open()) {
        text_out.flush();
        text_out.close();
    }
    sim_ptr = nullptr;
    enabled = false;
}

auto
Tracer::enable() -> void
{
    enabled = true;
}

auto
Tracer::disable() -> void
{
    enabled = false;
}

auto
Tracer::is_enabled() const -> bool
{
    return enabled;
}

auto
Tracer::instant(std::string component_name, std::string event_name, uint64_t txn_uid,
                std::optional<uint64_t> flit_id, std::optional<addr_t> address,
                std::map<std::string, std::string> args) -> void
{
    if (!enabled) {
        return;
    }
    assert(sim_ptr != nullptr && "Tracer::instant called before open");

    const time_ps now = sim_ptr->now();

    // JSONL output.
    if (jsonl_out.is_open()) {
        jsonl_out << "{";
        jsonl_out << R"("ts":)" << std::fixed << std::setprecision(3) << (now / 1000.0) << ",";
        jsonl_out << R"("pid":")" << txn_uid << "\",";
        jsonl_out << R"("tid":")";
        if (flit_id.has_value()) {
            jsonl_out << *flit_id;
        }
        else {
            jsonl_out << "0";
        }
        jsonl_out << R"(",)";
        jsonl_out << R"("cat":")" << escape_json_string(component_name) << "\",";
        jsonl_out << R"("name":")" << escape_json_string(event_name) << "\",";
        jsonl_out << R"("ph":"i")";

        const bool has_args = address.has_value() || !args.empty();
        if (has_args) {
            jsonl_out << R"(,"args":{)";
            bool first = true;
            if (address.has_value()) {
                jsonl_out << R"("address":")" << format_hex(*address) << "\"";
                first = false;
            }
            for (const auto& [k, v] : args) {
                if (!first)
                    jsonl_out << ",";
                jsonl_out << "\"" << escape_json_string(k) << "\":\"" << escape_json_string(v)
                          << "\"";
                first = false;
            }
            jsonl_out << "}";
        }

        jsonl_out << "}\n";
        jsonl_out.flush();
    }

    // Text output.
    if (text_out.is_open()) {
        text_out << "[t=" << format_ns(now) << "] " << component_name << " " << event_name
                 << " txn_uid=" << txn_uid;

        if (flit_id.has_value()) {
            text_out << " flit_id=" << *flit_id;
        }
        if (address.has_value()) {
            text_out << " addr=" << format_hex(*address);
        }
        for (const auto& [k, v] : args) {
            text_out << " " << k << "=" << v;
        }
        text_out << "\n";
        text_out.flush();
    }
}

} // namespace csim::tracing