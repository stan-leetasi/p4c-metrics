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

#ifndef LIB_OPTIONS_H_
#define LIB_OPTIONS_H_

#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cstring.h"
#include "error.h"
#include "map.h"

namespace P4::Util {

// Command-line options processing
class Options {
 public:
    enum OptionFlags {
        /// The default option flags.
        Default = 0,

        /// Hide this option from --help message.
        Hide = 1 << 0,

        /// If this option requires an argument, it may be omitted. Options with
        /// this flag set can only accept their argument with the syntax
        /// `--foo=bar`; `--foo bar` will be treated as if the argument to
        /// `--foo` were omitted. If the argument is omitted, null will be
        /// passed to the OptionProcessor.
        OptionalArgument = 1 << 1,
    };

    // return true if processing is successful
    using OptionProcessor = std::function<bool(const char *)>;

 private:
    void shortUsage();

 protected:
    struct Option {
        cstring option;
        const char *argName;  // nullptr if argument is not required
        const char *description;
        OptionProcessor processor;
        OptionFlags flags;
    };
    const char *binaryName;
    cstring message;
    // Build date and compile command required in couple runtime files
    cstring compileCommand;
    cstring buildDate;
    std::ostream *outStream = &std::cerr;

    std::map<cstring, const Option *> options;
    std::vector<cstring> optionOrder;
    std::vector<const char *> additionalUsage;
    std::vector<const char *> remainingOptions;  // produced as output
    // if true unknown options are collected in remainingOptions
    bool collectUnknownOptions = false;

    void setOutStream(std::ostream *out) { outStream = out; }
    void registerUsage(const char *msg) { additionalUsage.push_back(msg); }
    void registerOption(const char *option,         // option to register, e.g., -c or --version
                        const char *argName,        // name of option argument;
                                                    // nullptr if no argument expected
                        OptionProcessor processor,  // function to execute when option matches
                        const char *description,    // option help message
                        OptionFlags flags = OptionFlags::Default);  // additional flags

    explicit Options(std::string_view message)
        : binaryName(nullptr), message(message), compileCommand("") {}
    virtual ~Options() = default;

    /// Checks if parsed options make sense with respect to each-other.
    /// @returns true if the validation was successful and false otherwise.
    [[nodiscard]] virtual bool validateOptions() const;

 public:
    /**
     * Process options; return list of remaining options.
     * Also extracts the command name, startup time and saves a copy of the literal
     * commandline for future messages.
     *
     * If subclasses override this method, they should call the superclass
     * implementation.
     *
     * @return 'nullptr' if an error is signaled.
     */
    virtual std::vector<const char *> *process(int argc, char *const argv[]);

    /**
     * Process additional options, maybe not from the command line; return list
     * of remaining options.
     *
     * If subclasses override this method, they should call the superclass
     * implementation.
     *
     * @return 'nullptr' if an error is signaled.
     */
    virtual std::vector<const char *> *process_options(int argc, char *const argv[]);

    [[nodiscard]] virtual const char *getIncludePath() const = 0;
    cstring getCompileCommand() { return compileCommand; }
    cstring getBuildDate() { return buildDate; }
    cstring getBinaryName() { return cstring(binaryName); }
    virtual void usage();
};

}  // namespace P4::Util

#endif /* LIB_OPTIONS_H_ */
