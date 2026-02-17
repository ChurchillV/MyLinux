#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>

struct Options {
    bool number_lines = false;              // -n -> Number lines
    bool number_non_blank = false;          // -b -> Number non blank lines
    bool show_ends = false;                 // -E -> Display $ at the end of a line
    bool show_tabs = false;                 // -T -> Display I^ in place of tabs 
    bool squeeze_blanks = false;            // -s -> Squeeze multiple blank lines
};


/*
    UTILITY FUNCTION

    Display usage information
    
    Applies to: 
    
    '-h'
    @param program_name
*/
void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [OPTION]... [FILE]...\n"
              << "Concatenate FILE(S) to standard output. \n\n"
              << "Options:\n"
              << "  -n      number all output lines\n"
              << "  -b      number non-empty output lines\n"
              << "  -E      display $ at the end of each line\n"
              << "  -T      display TAB charactesr as ^I\n"
              << "  -s      squeeze multiple blank lines\n"
              << "  -h      display this help and exit\n"
              << "With no FILE or when FILE is -, read standard input.\n";    
}

/*
    HELPER FUNCTION

    Parses command line arguments, handles flags & filenames

    @param argc Argument count
    @param argv Arguments array
    @param opts Options struct (for handling flags)
    @param files Array of filenames passed as arguments
    @return success_status
*/
bool parse_arguments(int argc, char* argv[], Options& opts, std::vector<std::string>& files) {
    bool stop_parsing_flags = false;

    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // "--" means stop parsing flags
        if(arg == "--") {
            stop_parsing_flags = true;
            continue;
        }

        // Parse flags
        if(!stop_parsing_flags && arg[0] == '-' && arg.length() > 1) {
            // "-" means standard input
            if(arg == "-") {
                files.push_back(arg);
                continue;
            }

            // Handle single/combined flags
            for(size_t j = 1; j < arg.length(); ++j) {
                switch (arg[j])
                {
                case 'n':
                    opts.number_lines = true;
                    break;
                case 'b':
                    opts.number_non_blank = true;
                    break;
                case 'E':
                    opts.show_ends = true;
                    break;
                case 'T':
                    opts.show_tabs = true;
                    break;
                case 's':
                    opts.squeeze_blanks = true;
                    break;
                case 'h':
                    return false; // <- A signal to show help 
                default:
                    std::cerr << "mycat: invalid option -- '" << arg[j] << "'\n";
                    std::cerr << "Try mycat -h for more information.\n";
                    return false;
                }
            }
        } else {
            // Treat as a filename
            files.push_back(arg);
        }
    }

    // -b should override -n
    if(opts.number_non_blank) {
        opts.number_lines = false;
    }

    return true;
}

/*
    HELPER FUNCTION

    Checks if file exists and is readable

    @param filename
*/
bool check_file_access(const std::string& filename) {

    // stdin wil always be accessible
    if(filename == "-") {
        return true;
    }

    // File should exist
    struct stat file_stat;
    if(stat(filename.c_str(), &file_stat) != 0) {
        std::cerr << "mycat: " << filename << ": " << strerror(errno) << "\n";
        return false;
    }

    // Directory check
    if(S_ISDIR(file_stat.st_mode)) {    
        std::cerr << "mycat: " << filename << ": Is a directory\n";
        return false;
    }

    // Read permissions
    if(access(filename.c_str(), R_OK) != 0) {
        std::cerr << "mycat: " << filename << ": " << strerror(errno) << "\n";
        return false;
    }

    return true;
}

/*
    UTILITY FUNCTION

    Replace tabs with ^I and append $ at line endings based on flags
    
    Applies to 
    
    '-T'
    
    '-e'

    @param line Current line
    @param opts Flag options
*/
std::string transform_line(const std::string& line, const Options& opts) {
    std::string result = line;

    if(opts.show_tabs) {
        std::string transformed_line;
        for(char c : result) {
            if(c == '\t') {
                transformed_line += "^I";
            } else {
                transformed_line += c;
            }
        }
        result = transformed_line;
    }

    if(opts.show_ends) {
        result += "$";
    }

    return result;
}


/*
    HELPER + UTILITY FUNCTION

    Process a single file, apply line numbering, squeezing blanks

    Applies to

    '-s'

    '-n'

    '-b'

    @param filename
    @param opt Flag options
    @param line_number
    @param last_was_blank Tracks for consecutive blank lines
*/
bool process_file(const std::string& filename, const Options& opts, int& line_number, bool& last_was_blank) {
    std::istream* input;
    std::ifstream file_stream;

    if(filename == "-") {
        input = &std::cin;
    } else {
        file_stream.open(filename);
        if(!file_stream.is_open()) {
            std::cerr << "mycat: " << filename << ": " << strerror(errno) << "\n";
            return false;
        }
        input = &file_stream;
    }

    std::string line;
    while(std::getline(*input, line)) {
        bool is_blank = line.empty();

        // Apply -s (Squeeze multiple blank lines)
        if(opts.squeeze_blanks && is_blank && last_was_blank) {
            continue;
        }

        last_was_blank = is_blank;

        // Apply -n, -b (Line numbering)
        if(opts.number_lines || (opts.number_non_blank && !is_blank)) {
            std::cout << std::setw(6) << std::right << line_number++ << " ";
        }

        // Transform and output
        std::string transformed_line = transform_line(line, opts);
        std::cout << transformed_line << "\n";
    }


    // Check for read errors
    if(input->bad()) {
        std::cerr << "mycat: " << filename << ": Read error\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    Options opts;
    std::vector<std::string> files;

    // Parse command line arguments
    if(!parse_arguments(argc, argv, opts, files)) {
        print_usage(argv[0]);
        return 1;
    }

    // Read from stdin in case of no files
    if(files.empty()) {
        files.push_back("-");
    }

    // Process files
    int line_number = 1;
    bool last_was_blank = false;
    bool had_errors = false;

    for(const auto& filename : files) {
        // Check file access, skip for stdin
        if(filename != "-" && !check_file_access(filename)) {
            had_errors= true;
            continue;
        }

        // Process the file
        if(!process_file(filename, opts, line_number, last_was_blank)) {
            had_errors = true;
        }
    }

    return had_errors ? 1 : 0;
}