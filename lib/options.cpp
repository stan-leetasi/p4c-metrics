/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "options.h"

#include "lib/null.h"

namespace P4 {

void Util::Options::shortUsage() {
    *outStream << "Use '--help' to see all available options." << std::endl;
}

void Util::Options::registerOption(const char *option, const char *argName,
                                   OptionProcessor processor, const char *description,
                                   OptionFlags flags /* = OptionFlags::Default */) {
    if (option == nullptr || processor == nullptr || description == nullptr)
        throw std::logic_error("Null argument to registerOption");
    if (strlen(option) <= 1) throw std::logic_error(std::string("Option too short: ") + option);
    if (option[0] != '-')
        throw std::logic_error(std::string("Expected option to start with -: ") + option);
    auto o = new Option();
    o->option = cstring(option);
    o->argName = argName;
    o->processor = processor;
    o->description = description;
    o->flags = flags;
    auto opt = get(options, cstring(option));
    if (opt != nullptr) throw std::logic_error(std::string("Option already registered: ") + option);
    options.emplace(option, o);
    optionOrder.push_back(cstring(option));
}

// Process options; return list of remaining options.
// Returns 'nullptr' if an error is signalled
std::vector<const char *> *Util::Options::process(int argc, char *const argv[]) {
    if (argc == 0 || argv == nullptr) throw std::logic_error("No arguments to process");
    binaryName = argv[0];
    // collect command line args
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            compileCommand += argv[i];
            compileCommand += " ";
        }
        compileCommand = compileCommand.trim();
    }
    // collect program compilation date
    const time_t now = time(NULL);
    char build_date[50];
    strftime(build_date, 50, "%c", localtime(&now));
    buildDate = cstring(build_date);
    return process_options(argc, argv);
}

std::vector<const char *> *Util::Options::process_options(int argc, char *const argv[]) {
    for (int i = 1; i < argc; i++) {
        cstring opt = cstring(argv[i]);
        const char *arg = nullptr;
        const Option *option = nullptr;

        if (opt.startsWith("--")) {
            option = get(options, opt);
            if (!option && (arg = opt.find('='))) option = get(options, opt.before(arg++));
            if (option == nullptr) {
                ::P4::error(ErrorType::ERR_UNKNOWN, "Unknown option %1%", opt);
                shortUsage();
                return nullptr;
            }
        } else if (opt.startsWith("-") && opt.size() > 1) {
            // Support GCC-style long options that begin with a single '-'.
            option = get(options, opt);

            // If there's no such option, try single-character options.
            if (option == nullptr && opt.size() > 2) {
                arg = opt.substr(2).c_str();
                opt = opt.substr(0, 2);
                option = get(options, opt);
            }
            if (option == nullptr) {
                ::P4::error(ErrorType::ERR_UNKNOWN, "Unknown option %1%", opt);
                shortUsage();
                return nullptr;
            }
            if ((option->flags & OptionFlags::OptionalArgument) && (!arg || strlen(arg) == 0))
                arg = nullptr;
        }

        if (option == nullptr) {
            remainingOptions.push_back(opt.c_str());
        } else {
            if (option->argName != nullptr && arg == nullptr &&
                !(option->flags & OptionFlags::OptionalArgument)) {
                if (i == argc - 1) {
                    ::P4::error(ErrorType::ERR_EXPECTED,
                                "Option %1% is missing required argument %2%", opt,
                                option->argName);
                    shortUsage();
                    return nullptr;
                }
                arg = argv[++i];
            }
            bool success = option->processor(arg);
            if (!success) {
                shortUsage();
                return nullptr;
            }
        }
    }

    auto result = validateOptions();
    if (!result) {
        shortUsage();
        return nullptr;
    }

    return &remainingOptions;
}

void Util::Options::usage() {
    *outStream << binaryName << ": " << message << std::endl;

    size_t labelLen = 0;
    for (const auto &o : optionOrder) {
        size_t len = o.size();
        auto option = get(options, o);
        CHECK_NULL(option);
        if (option->argName != nullptr) len += 1 + strlen(option->argName);
        if (labelLen < len) labelLen = len;
    }

    labelLen += 3;
    for (const auto &o : optionOrder) {
        auto option = get(options, o);
        CHECK_NULL(option);
        size_t len = o.size();
        if (option->flags & OptionFlags::Hide) continue;
        *outStream << option->option;
        if (option->argName != nullptr) {
            if (option->flags & OptionFlags::OptionalArgument) {
                *outStream << "[=" << option->argName << "]";
                len += 3 + strlen(option->argName);
            } else {
                *outStream << " " << option->argName;
                len += 1 + strlen(option->argName);
            }
        }
        std::string line;
        std::stringstream ss(option->description);
        while (std::getline(ss, line)) {
            *outStream << std::string(labelLen - len, ' ');
            *outStream << line << std::endl;
            len = 0;
        }
    }

    if (additionalUsage.size() > 0) {
        *outStream << "Additional usage instructions:" << std::endl;
    }
    for (auto m : additionalUsage) *outStream << m << std::endl;
}

bool Util::Options::validateOptions() const { return true; }

}  // namespace P4
