// The only C++ in the project that knows about Emscripten.
// Exposes core functions to JavaScript through Embind.

#include <string>

#include <emscripten/bind.h>

#include "core/version.h"

namespace {

// Embind converts std::string to a JS string, but not std::string_view.
std::string version() {
    return std::string(chess::version());
}

} // namespace

EMSCRIPTEN_BINDINGS(chess) {
    emscripten::function("version", &version);
}
