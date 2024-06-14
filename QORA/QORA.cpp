#include "pch.h"

#include "Environment.h"
#include "Domains.h"
#include "LearnerQORA.h"

#include "util.h"
#include "Parameters.h"
#include "Serialization.h"

////////////////////////////////////////////////////////////////////////////////
//benchmarking options
////////////////////////////////////////////////////////////////////////////////

struct DomainConstructor {
    typedef std::function<Environment* (const std::map<std::string, std::string>&)> Constructor;
    //
    Parameters params;
    Constructor constructor; //construct an environment on the heap and return it, using the given cmdline parameters
};

struct LearnerConstructor {
    typedef std::function<Learner* (const Environment* env, const std::map<std::string, std::string>&)> Constructor;
    //
    Parameters params;
    Constructor constructor; //construct a learner on the heap and return it, using the given cmdline parameters
};

struct Contents {
    //available domains, with their parameters
    std::vector<std::string> domain_names;
    std::map<std::string, DomainConstructor> domains;
    void addDomain(const std::string& name, const Parameters& params, const DomainConstructor::Constructor& constructor) {
        domain_names.push_back(name);
        domains[name] = DomainConstructor{ params, constructor };
    }
    //learners
    std::vector<std::string> learner_names;
    std::map<std::string, LearnerConstructor> learners;
    void addLearner(const std::string& name, const Parameters& params, const LearnerConstructor::Constructor& constructor) {
        learner_names.push_back(name);
        learners[name] = LearnerConstructor{ params, constructor };
    }
};

Contents CONTENTS; //initialized by main

////////////////////////////////////////////////////////////////////////////////
//usage info
////////////////////////////////////////////////////////////////////////////////

void print_usage(const char* exe) {
    fprintf(stderr, "Usage: %s <mode> [args]\n", exe);
    fprintf(stderr, "\
  Modes:\n\
\n\
  * help (or blank): Print usage information, list available testing domains/learners\n\
\n\
  * play <domain>: Interact with a domain\n\
\n\
  * types <domain> [format: default|json]: Output the types in a domain\n\
\n\
  * gen <n> <m> <domain> <file> [k=1]: Generates a random set of environments\n\
     and actions in the given domain for later testing\n\
    - Generates n levels, with m random actions per level, for a total of n*m observed transitions\n\
    - Domain arguments can be specified as domain(arg1:a,arg2:b,...)\n\
    - If m=0, this will just generate a list of starting states (action lists will be empty)\n\
       for use with planning benchmarks\n\
    - Stores the data in the following format:\n\
        Levels: object{identifier: \"states_concise\", domain: string, parameters: object,\n\
         levels: list[Level]}\n\
        Level: object{start: list[Object], actions: list[int]}\n\
        Object: object{id: int, type: int, attributes: object}\n\
    - If k>1, the above routine is run k times and the given file name is treated as a stem\n\
       so the actual filenames will be <file>_i.txt\n\
\n\
  * verbose <input file> <output file> [k=1]: Takes a pre-generated concise observations file (from gen) \n\
     and runs the domain logic to convert it to a list of (s, a, s') transitions\n\
     for use in external training programs that don't have domain logic programmed in\n\
    - Stores the data in the following format:\n\
        Observations: object{identifier: \"states_verbose\", domain: string, parameters: object,\n\
         observations: list[Observation]}\n\
        Observation: object{start: list[Object], action: int, next: list[Object]}\n\
        Object: object{id: int, type: int, attributes: object}\n\
    - If k>1, the above routine is run k times and the given file names are treated as stems\n\
       so the actual filenames will be <file>_i.txt\n\
\n\
  * flatten <input file> <output file> [k=1]: Takes a pre-generated observations file\n\
     and uses the domain's types to flatten the object-oriented representation\n\
     into a WxHxD grid suitable for use with neural networks\n\
    - If k>1, the above routine is run k times and the given file names are treated as stems\n\
       so the actual filenames will be <file>_i.txt\n\
\n\
  * view <file>: Takes a pre-generated observations file (from gen/verbose) \n\
     and allows the user to step through (s, a, s') observation triplets one-at-a-time\n\
\n\
  * predict <learner(s)> <model file name stem> <data output file> <input file> [k=1]:\n\
     Feeds the observations from a pre-generated list of states\n\
     (concise or verbose) to a set of learners and evaluates their prediction accuracies\n\
     (prints the prediction error of each observation)\n\
    - Learner arguments can be specified as learner(arg1:a,arg2:b,...)\n\
    - Multiple learners must be separated by semicolons\n\
    - The final learned models are saved to disk for later use\n\
       with filenames <model file name>_<learner index>.json if k=1\n\
       or <model file name>_<learner index>_<training index>.json if k>1\n\
    - If k>1, the above routine is run k times and the given input/output file names are treated as stems\n\
       so the actual filenames will be <file>_i.txt\n\
\n\
  * predict_pt <learner file(s)> <model file name stem> <data output file> <observations file>\
     [learning enabled, true|false, default=false] [k=1]:\n\
     Load pre-trained models from the disk and evaluate them on observations loaded from a pre-generated file\n\
    - If learning is enabled, the updated models are saved to disk for later use\n\
       with filenames <model file name>_<learner index>.json if k=1\n\
       or <model file name>_<learner index>_<training index>.json if k>1\n\
    - Multiple learner filename stems can be given, separated by semicolons\n\
    - If k>1, the above routine is run k times and the given learner/input/output file names are treated as stems\n\
       so the actual filenames will be <file>_i.txt\n\
\n\
  * avg <output file> <data file> [k=1] [n=1]: Takes a set of prediction error files and averages them \n\
     so that all of the data collected for a given learner over each run is combined\n\
     and groups of n consecutive observations are also combined to condense the data\n\
\n\
  * print <learner file>: Print a learned model's parameters\n\
\n\
  * test <learner> <domain> <m>: Tests a learning algorithm on a domain,\n\
     keeping track of prediction error over time to get an estimate\n\
     of how many observations that algorithm needs to learn that domain\n\
\n\
  * plan <learner> <domain> <n> <m>: Use a learner to generate plans via BFS state-space search\n\
\n\
  * exec <n> <m> <k> <domain> <stem> <learner(s)> [n_avg=1]: Shorthand for\n\
     gen n m domain levels/stem k\n\
     predict learners models/stem data/stem levels/stem k\n\
     avg data/avg_stem.txt data/stem k n_avg\n\
\n\
  * exec_t <n> <m> <k> <domain> <stem> <n2> <m2> <domain2> <stem2> <learner(s)> [learning=false] [n_avg=1]: Shorthand for\n\
     gen n m domain levels/stem k\n\
     predict learners models/stem data/stem levels/stem k\n\
     avg data/avg_stem.txt data/stem k n_avg\n\
     gen n2 m2 domain2 levels/stem2 k\n\
     predict_pt models/stem_0... models/stem2_t(l) data/stem2_t(l) levels/stem2 <learning?> k\n\
     avg data/avg_stem2_t(l).txt data/stem2_t(l) k n_avg\n\
");

    //list available domains
    fprintf(stderr, "\nDomains:\n");
    for (const std::string name : CONTENTS.domain_names) {
        const Parameters& params = CONTENTS.domains.at(name).params;
        fprintf(stderr, "\n  %s\n", name.c_str());
        for (int i = 0; i < params.size(); i++) {
            fprintf(stderr, "    %s\n", params.getParameter(i).to_string().c_str());
        }
    }

    //list available learners
    fprintf(stderr, "\nLearners:\n");
    for (const std::string name : CONTENTS.learner_names) {
        const Parameters& params = CONTENTS.learners.at(name).params;
        fprintf(stderr, "\n  %s\n", name.c_str());
        for (int i = 0; i < params.size(); i++) {
            fprintf(stderr, "    %s\n", params.getParameter(i).to_string().c_str());
        }
    }

    fprintf(stderr, "\n");
}

////////////////////////////////////////////////////////////////////////////////
//mode implementations
////////////////////////////////////////////////////////////////////////////////

int run_play(int argc, char** argv) {
    //check arg
    if (argc < 1) {
        Logger::log("play needs 1 argument: domain", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string domain_nameargs = argv[0];
    //parse domain stuff
    auto namepair = str_args(domain_nameargs);
    std::string domain_name = namepair.first;
    std::string domain_args = namepair.second;
    auto it = CONTENTS.domains.find(domain_name);
    if (it == CONTENTS.domains.end()) {
        Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
        return EXIT_FAILURE;
    }
    DomainConstructor& constructor = it->second;
    std::map<std::string, std::string> args;
    if (!constructor.params.parse(domain_args, args)) {
        return EXIT_FAILURE;
    }

    //create the environment and rng
    Environment* env = constructor.constructor(args);
    Types& types = env->getTypes();
    Random random;
    random.seed_time();
    //
    State state = env->createRandomState(random);
    int observations = 0;
    while (true) {
        //print current state
        fprintf(stderr, "Observation: %d\n\n", observations);
        //
        env->print(state);

        //prompt user for input
        fprintf(stderr, "\nActions:\n");
        for (const Action& a : types.getActions()) {
            fprintf(stderr, " [%d] %s\n", a.id, a.name.c_str());
        }
        //
        fprintf(stderr, "Enter action or exit: ");
        //
        std::string line;
        std::getline(std::cin, line);
        //uppercase, conveniently taken from https://stackoverflow.com/a/17793588 and https://stackoverflow.com/a/735217
        for (char& c : line) c = toupper((unsigned char)c);
        ActionId chosen = -1;
        if (line == "EXIT") {
            fprintf(stderr, "Exiting\n");
            break;
        }
        else if (line == "") {
            //random action
            chosen = random.sample(types.getActions()).id;
        }
        else {
            //try to parse action; on failure, continue
            try {
                chosen = types.getActionByName(line);
            }
            catch(...) {
                try {
                    chosen = std::stoi(line);
                }
                catch (...) {
                    fprintf(stderr, "Failed to parse \"%s\"\n", line.c_str());
                    continue;
                }
            }
        }
        fprintf(stderr, "Chosen action: %d %s\n", chosen, types.getActions().at(chosen).name.c_str());

        //act
        state = env->act(state, chosen, random).sample(random);

        //
        observations++;
        fprintf(stderr, "\n");
    }
    //
    delete env;
    //
    return EXIT_SUCCESS;
}

int run_types(int argc, char** argv) {
    //check arg
    if (argc < 1) {
        Logger::log("types needs 1-2 arguments: domain [format]", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string domain_nameargs = argv[0];
    std::string format = "default";
    if (argc > 1) {
        format = argv[1];
    }
    //parse domain stuff
    auto namepair = str_args(domain_nameargs);
    std::string domain_name = namepair.first;
    std::string domain_args = namepair.second;
    auto it = CONTENTS.domains.find(domain_name);
    if (it == CONTENTS.domains.end()) {
        Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
        return EXIT_FAILURE;
    }
    DomainConstructor& constructor = it->second;
    std::map<std::string, std::string> args;
    if (!constructor.params.parse(domain_args, args)) {
        return EXIT_FAILURE;
    }

    //create the environment
    Environment* env = constructor.constructor(args);
    Types& types = env->getTypes();
    //
    if (format == "default") {
        types.print();
    }
    else if (format == "json") {
        std::cout << std::setw(2) << types.to_json() << std::endl;
    }
    else {
        Logger::log(Logger::formatString("Unsupported format \"%s\"", format.c_str()), true);
        return EXIT_FAILURE;;
    }
    //
    delete env;
    //
    return EXIT_SUCCESS;
}

int run_gen(int n, int m, const std::string& domain_nameargs, const std::string& file, int k) {
    //parse domain stuff
    auto namepair = str_args(domain_nameargs);
    std::string domain_name = namepair.first;
    std::string domain_args = namepair.second;
    auto it = CONTENTS.domains.find(domain_name);
    if (it == CONTENTS.domains.end()) {
        Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
        return EXIT_FAILURE;
    }
    DomainConstructor& constructor = it->second;
    std::map<std::string, std::string> args;
    if (!constructor.params.parse(domain_args, args)) {
        return EXIT_FAILURE;
    }

    //create the environment and rng
    Environment* env = constructor.constructor(args);
    Types& types = env->getTypes();

    //init info about the domain stuff being generated
    json file_info{
        {"identifier", "states_concise"},
        {"domain", domain_name},
        {"parameters", args},
        {"n", n},
        {"m", m},
        {"k", k}
    };

    Progress progress_k("Generating sequences", k);
    Logger::indent_push();
    for (int index = 0; index < k; index++) {
        std::string filename = (k == 1) ? file : (file + "_" + std::to_string(index) + ".txt");
        std::ofstream output(filename);
        if (!output.good()) {
            Logger::log(Logger::formatString("Failed to open output file \"%s\"", filename.c_str()), true);
            return EXIT_FAILURE;
        }


        file_info["index"] = index;
        //
        Random random;
        random.seed_time(index);
        file_info["random_seed"] = random.get_seed();
        //
        output << std::setw(2) << file_info << std::endl;

        //now iterate and create all the levels
        Progress progress_n(Logger::formatString("Sequence %d/%d", index + 1, k), n);
        Logger::indent_push();
        for (int i = 0; i < n; i++) {

            //generate a level
            State state = env->createRandomState(random);
            std::vector<ActionName> actions(m);
            for (int j = 0; j < m; j++) {
                //generate an action
                actions[j] = random.sample(types.getActions()).name;
            }
            //output level and actions
            json level_info{
                {"level", env->getTypes().to_json(state)},
                {"actions", actions}
            };
            output << std::setw(2) << level_info << std::endl;

            if (n > 1) {
                progress_n.update(i + 1);
            }
        }
        if (n > 1) {
            progress_n.end();
        }
        Logger::indent_pop();

        output.close();

        if (k > 1) {
            progress_k.update(index + 1);
        }
    }
    if (k > 1) {
        progress_k.end();
    }
    Logger::indent_pop();
    //
    delete env;
    //
    return EXIT_SUCCESS;
}

//gen <n> <m> <domain> <file> [k=1]
int run_gen(int argc, char** argv) {
    //check args
    if (argc < 4) {
        Logger::log("gen needs 4-5 arguments: n, m, domain, file, [k=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    int n = atoi(argv[0]); //number of levels (start states) to generate
    int m = atoi(argv[1]); //number of actions to generate per start state
    std::string domain_nameargs = argv[2]; //domain(args)
    std::string filename = argv[3];
    int k = 1;
    if (argc > 4) k = atoi(argv[4]);

    return run_gen(n, m, domain_nameargs, filename, k);
}

int run_verbose(const std::string& file_in, const std::string& file_out, int k) {
    Progress progress_k("Flattening", k);
    Logger::indent_push();
    for (int index = 0; index < k; index++) {
        std::string filename_in = (k == 1) ? file_in : (file_in + "_" + std::to_string(index) + ".txt");
        std::string filename_out = (k == 1) ? file_out : (file_out + "_" + std::to_string(index) + ".txt");

        //attempt to open the files
        std::ifstream input(filename_in);
        if (!input.good()) {
            Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
            return EXIT_FAILURE;
        }
        std::ofstream output(filename_out);
        if (!output.good()) {
            Logger::log(Logger::formatString("Failed to open output file \"%s\"", filename_out.c_str()), true);
            return EXIT_FAILURE;
        }
        //read the generated level set metadata / domain info
        json file_info;
        input >> file_info;
        if (file_info["identifier"] != "states_concise") {
            Logger::log(Logger::formatString("verbose can't expand file of type \"%s\"", file_info["identifier"].get<std::string>().c_str()), true);
            return EXIT_FAILURE;
        }
        int n = file_info["n"].get<int>(); //number of levels
        int m = file_info["m"].get<int>(); //number of actions per level
        //
        const std::string& domain_name = file_info["domain"].get<std::string>();
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        const json& domain_parameters = file_info["parameters"];
        Environment* env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>());
        const Types& types = env->getTypes();
        //output the new file data
        json file_info_v{
            {"identifier", "states_verbose"},
            {"domain", domain_name},
            {"parameters", domain_parameters},
            {"n", n * m},
            {"random_seed", file_info["random_seed"]}
        };
        output << std::setw(2) << file_info_v << std::endl;
        //go through the generated levels and expand the observations
        ObservationIterator observations(input, env, file_info);
        Progress progress(Logger::formatString("Flatten %d/%d", index + 1, k), observations.count());
        Logger::indent_push();
        while (observations.next()) {
            //
            const State& s = observations.getStartState();
            const ActionName& aname = observations.getAction();
            const State& s_prime = observations.getNextState();
            const StateDistribution& s_primes = observations.getNextStates();
            //
            json obj_out{
                {"start", types.to_json(s)},
                {"action", aname},
                {"next", types.to_json(s_prime)}
            };
            types.to_json(s_primes, obj_out["nexts"]);
            //
            output << std::setw(2) << obj_out << std::endl;
            //
            if (observations.count() > 1) progress.update(observations.index() + 1);
        }
        if (observations.count() > 1) progress.end();
        Logger::indent_pop();
        //
        delete env;
        //
        input.close();
        output.close();

        //
        if(k > 1) progress_k.update(index + 1);
    }
    if (k > 1) progress_k.end();
    Logger::indent_pop();
}

//verbose <input file> <output file> [k=1]
int run_verbose(int argc, char** argv) {
    //check args
    if (argc < 2) {
        Logger::log("verbose needs 2-3 arguments: <input file> <output file> [k=1]", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string file_in = argv[0];
    std::string file_out = argv[1];
    int k = 1;
    if (argc > 2) k = atoi(argv[2]);
    //
    return run_verbose(file_in, file_out, k);
}

int run_flatten(const std::string& file_in, const std::string& file_out, int k) {
    for (int index = 0; index < k; index++) {
        std::string filename_in = (k == 1) ? file_in : (file_in + "_" + std::to_string(index) + ".txt");
        std::string filename_out = (k == 1) ? file_out : (file_out + "_" + std::to_string(index) + ".txt");

        //attempt to open the files
        std::ifstream input(filename_in);
        if (!input.good()) {
            Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
            return EXIT_FAILURE;
        }
        std::ofstream output(filename_out);
        if (!output.good()) {
            Logger::log(Logger::formatString("Failed to open output file \"%s\"", filename_out.c_str()), true);
            return EXIT_FAILURE;
        }
        //read the generated level set metadata / domain info
        json file_info;
        input >> file_info;
        //
        const std::string& domain_name = file_info["domain"].get<std::string>();
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        const json& domain_parameters = file_info["parameters"];
        Environment* env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>()); 
        const Types& types = env->getTypes();
        //go through the generated levels and expand the observations
        ObservationIterator observations(input, env, file_info);
        //output the new file data
        json file_info_v{
            {"identifier", "states_flat"},
            {"domain", domain_name},
            {"parameters", domain_parameters},
            {"types", types.to_json()}, //for convenience of the python program
            {"n", observations.count()},
            {"random_seed", file_info["random_seed"]}
        };
        output << std::setw(2) << file_info_v << std::endl;
        while (observations.next()) {
            //
            const State& s = observations.getStartState();
            const ActionName& aname = observations.getAction();
            const State& s_prime = observations.getNextState();
            //
            auto s_flat = env->flatten(s);
            ActionId a = types.getActionByName(aname);
            auto sp_flat = env->flatten(s_prime);
            //
            json obj_out{
                {"start", {
                    {"grid", s_flat.first.getValues()},
                    {"data", s_flat.second}
                }},
                {"action", a},
                {"next", {
                    {"grid", sp_flat.first.getValues()},
                    {"data", sp_flat.second}
                }}
            };
            //output << std::setw(2) << obj_out << std::endl;
            output << obj_out << std::endl; //condense whitespace
        }
        //
        delete env;
        //
        input.close();
        output.close();
    }
}

//verbose <input file> <output file> [k=1]
int run_flatten(int argc, char** argv) {
    //check args
    if (argc < 2) {
        Logger::log("flatten needs 2-3 arguments: <input file> <output file> [k=1]", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string file_in = argv[0];
    std::string file_out = argv[1];
    int k = 1;
    if (argc > 2) k = atoi(argv[2]);
    //
    return run_flatten(file_in, file_out, k);
}

int run_view(int argc, char** argv) {
    //check arg
    if (argc < 1) {
        Logger::log("view needs 1 argument: input file", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string filename = argv[0];
    //attempt to open the file
    std::ifstream input(filename);
    if (!input.good()) {
        Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename.c_str()), true);
        return EXIT_FAILURE;
    }
    //read the generated level set metadata / domain info
    json file_info;
    input >> file_info;
    //
    const std::string& domain_name = file_info["domain"].get<std::string>();
    auto it = CONTENTS.domains.find(domain_name);
    if (it == CONTENTS.domains.end()) {
        Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
        return EXIT_FAILURE;
    }
    DomainConstructor& constructor = it->second;
    const json& domain_parameters = file_info["parameters"];
    Environment* env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>());
    Types& types = env->getTypes();
    //go through the generated levels and expand the observations
    ObservationIterator observations(input, env, file_info);
    while (observations.next()) {
        //
        const State& s = observations.getStartState();
        const ActionName& aname = observations.getAction();
        const State& s_prime = observations.getNextState();
        //
        printf("\n====================\nObservation %d/%d\n====================\n\n", observations.index() + 1, observations.count());
        env->print(s);
        printf("\nAction: %d %s\n\n", types.getActionByName(aname), aname.c_str());
        env->print(s_prime);
        
        getchar();
    }
    //
    delete env;
    //
    return EXIT_SUCCESS;
}

int run_predict(const std::string& learner_list, const std::string& file_models, const std::string& file_out, const std::string& file_in, int k) {
    //preemptively do some parsing of the learners
    std::vector<LearnerConstructor*> learner_constructors;
    std::vector<std::string> learner_names;
    std::vector<std::map<std::string, std::string>> learner_params;
    for (const std::string& learner_str : str_split(learner_list, ";")) {
        auto namepair = str_args(learner_str);
        std::string learner_name = namepair.first;
        std::string learner_args = namepair.second;
        //
        auto it = CONTENTS.learners.find(learner_name);
        if (it == CONTENTS.learners.end()) {
            Logger::log(Logger::formatString("Learner not found: \"%s\"", learner_name.c_str()), true);
            return EXIT_FAILURE;
        }
        LearnerConstructor& constructor = it->second;
        std::map<std::string, std::string> args;
        if (!constructor.params.parse(learner_args, args)) {
            return EXIT_FAILURE;
        }
        learner_constructors.push_back(&constructor);
        learner_names.push_back(learner_name);
        learner_params.push_back(args);
    }

    //
    Random random;
    random.seed_time();
    //
    Progress progress_k("Predict", k);
    Logger::indent_push();
    for (int index = 0; index < k; index++) {
        std::string filename_in = (k == 1) ? file_in : (file_in + "_" + std::to_string(index) + ".txt");
        std::string filename_out = (k == 1) ? file_out : (file_out + "_" + std::to_string(index) + ".txt");

        //attempt to open the files
        std::ifstream input(filename_in);
        if (!input.good()) {
            Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
            return EXIT_FAILURE;
        }
        std::ofstream output(filename_out);
        if (!output.good()) {
            Logger::log(Logger::formatString("Failed to open output file \"%s\"", filename_out.c_str()), true);
            return EXIT_FAILURE;
        }

        //read the generated level set metadata / domain info
        json file_info;
        input >> file_info;
        const std::string& domain_name = file_info["domain"].get<std::string>();
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        const json& domain_parameters = file_info["parameters"];
        Environment* env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>());
        const Types& types = env->getTypes();

        //construct learners
        std::vector<Learner*> learners;
        for (int i = 0; i < learner_constructors.size(); i++) {
            Learner* learner = learner_constructors[i]->constructor(env, learner_params[i]);
            learners.push_back(learner);
        }

        //run through the observations
        ObservationIterator observations(input, env, file_info);
        int observation_count = 1;
        Progress progress(Logger::formatString("Sequence %d/%d", index + 1, k), observations.count());
        Logger::indent_push();
        while (observations.next()) {
            const State& s = observations.getStartState();
            State sHidden = env->hideInformation(s);
            const ActionName& aname = observations.getAction();
            ActionId a = types.getActionByName(aname);
            const StateDistribution& s_primes = observations.getNextStates();
            State sHidden_prime = env->hideInformation(observations.getNextState());
            //print out current observation number
            //printf("%d\t", observation_count);
            output << observation_count;
            //run prediction of each learner, evaluate and output error
            for (Learner* learner : learners) {
                StateDistribution predicted_states = learner->predictTransition(sHidden, a, random);
                double error = s_primes.error(predicted_states);
                //printf("%d\t", error);
                output << '\t' << error;
            }
            //printf("\n");
            output << '\n';
            //update each learner
            for (Learner* learner : learners) {
                learner->observeTransition(sHidden, a, sHidden_prime);
            }
            //
            observation_count++;
            //
            if (observations.count() > 1) progress.update(observations.index() + 1);
        }
        if (observations.count() > 1) progress.end();
        Logger::indent_pop();
        //output each learned model to disk
        //just put them all into files model_<timestamp>_<pid>_learner<#>.json
        //with learner name + params so they can be reconstructed
        //and some metadata (what domain they were trained in, how many observations they trained on, ...)
        //std::string str_timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        //std::string str_pid = std::to_string(GetCurrentProcessId());
        //std::string learner_filename_prefix = "model_" + str_timestamp + "_" + str_pid + "_learner";
        for (int i = 0; i < learners.size(); i++) {
            Learner* learner = learners[i];
            json learner_data{
                {"name", learner_names[i]},
                {"parameters", learner_params[i]},
                {"domain", {
                    {"name", domain_name},
                    {"parameters", domain_parameters}
                }},
                {"observations", observation_count - 1},
                {"model", learner->to_json()}
            };
            //
            std::string str_id = std::to_string(i);
            std::string model_filename = (k == 1) ? (file_models + "_" + str_id + ".json") : (file_models + "_" + str_id + "_" + std::to_string(index) + ".json");
            std::ofstream output_learner(model_filename);
            if (!output_learner.good()) {
                Logger::log(Logger::formatString("Failed to open model output file \"%s\"", model_filename.c_str()), true);
                return EXIT_FAILURE;
            }
            output_learner << std::setw(2) << learner_data << std::endl;
            output_learner.close();
            //
            delete learner;
        }

        //
        delete env;
        //
        input.close();
        output.close();

        //
        if (k > 1) progress_k.update(index + 1);
    }
    if (k > 1) progress_k.end();
    Logger::indent_pop();
}

//predict <learner(s)> <model file name stem> <data output file> <input file> [k=1]
int run_predict(int argc, char** argv) {
    //check args
    if (argc < 4) {
        Logger::log("predict needs 4-5 arguments: <learner(s)> <model file> <data output file> <input file> [k=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    std::string learner_list = argv[0];
    std::string file_models = argv[1];
    std::string file_output = argv[2];
    std::string file_input = argv[3];
    int k = 1;
    if (argc > 4) k = atoi(argv[4]);
    //
    return run_predict(learner_list, file_models, file_output, file_input, k);
}

int run_predict_pt(const std::string& learner_list, const std::string& file_models, const std::string& file_out, const std::string& file_in, bool learning_enabled, int k) {
    //load the learners
    std::vector<std::string> learner_files = str_split(learner_list, ";");

    //
    Random random;
    random.seed_time();
    //
    Progress progress_k("Predict_pt", k);
    Logger::indent_push();
    for (int index = 0; index < k; index++) {

        std::string filename_in = (k == 1) ? file_in : (file_in + "_" + std::to_string(index) + ".txt");
        std::string filename_out = (k == 1) ? file_out : (file_out + "_" + std::to_string(index) + ".txt");

        //attempt to open the files
        std::ifstream input(filename_in);
        if (!input.good()) {
            Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
            return EXIT_FAILURE;
        }
        std::ofstream output(filename_out);
        if (!output.good()) {
            Logger::log(Logger::formatString("Failed to open output file \"%s\"", filename_out.c_str()), true);
            return EXIT_FAILURE;
        }

        //read the generated level set metadata / domain info
        json file_info;
        input >> file_info;
        const std::string& domain_name = file_info["domain"].get<std::string>();
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        const json& domain_parameters = file_info["parameters"];
        Environment* env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>());
        const Types& types = env->getTypes();

        //construct learners from files
        std::vector<Learner*> learners;
        std::vector<json> learner_datas; //json files loaded from each learned model input
        for (int i = 0; i < learner_files.size(); i++) {
            const std::string& learner_file_stem = learner_files[i];
            std::string filename = (k == 1) ? learner_file_stem : (learner_file_stem + "_" + std::to_string(index) + ".json");
            //attempt to open the file
            std::ifstream input_learner(filename);
            if (!input_learner.good()) {
                Logger::log(Logger::formatString("Failed to open model input file \"%s\"", filename.c_str()), true);
                return EXIT_FAILURE;
            }
            //read the learner data
            json learner_data;
            input_learner >> learner_data;
            learner_datas.push_back(learner_data);
            //
            std::string learner_name = learner_data["name"].get<std::string>();
            auto it = CONTENTS.learners.find(learner_name);
            if (it == CONTENTS.learners.end()) {
                Logger::log(Logger::formatString("Learner not found: \"%s\"", learner_name.c_str()), true);
                return EXIT_FAILURE;
            }
            LearnerConstructor& constructor = it->second;
            const json& learner_parameters = learner_data["parameters"];
            Learner* learner = constructor.constructor(env, learner_parameters.get<std::map<std::string, std::string>>());
            learner->from_json(learner_data["model"]);
            //
            learners.push_back(learner);
            input_learner.close();
        }

        //run through the observations
        ObservationIterator observations(input, env, file_info);
        int observation_count = 1;
        Progress progress(Logger::formatString("Sequence %d/%d", index + 1, k), observations.count());
        Logger::indent_push();
        while (observations.next()) {
            const State& s = observations.getStartState();
            State sHidden = env->hideInformation(s);
            const ActionName& aname = observations.getAction();
            ActionId a = types.getActionByName(aname);
            const StateDistribution& s_primes = observations.getNextStates();
            State sHidden_prime = env->hideInformation(observations.getNextState());
            //print out current observation number
            //printf("%d\t", observation_count);
            output << observation_count;
            //run prediction of each learner, evaluate and output error
            for (Learner* learner : learners) {
                StateDistribution predicted = learner->predictTransition(sHidden, a, random);
                double error = s_primes.error(predicted);
                //printf("%d\t", error);
                output << '\t' << error;
            }
            //printf("\n");
            output << '\n';
            //update each learner
            if (learning_enabled) {
                for (Learner* learner : learners) {
                    learner->observeTransition(sHidden, a, sHidden_prime);
                }
            }
            //
            observation_count++;
            //
            if (observations.count() > 1) progress.update(observations.index() + 1);
        }
        if (observations.count() > 1) progress.end();
        Logger::indent_pop();
        for (int i = 0; i < learners.size(); i++) {
            Learner* learner = learners[i];
            const json& learner_data_prev = learner_datas[i];
            if (learning_enabled) {
                //output each learned model to disk
                //just put them all into files model_<timestamp>_<pid>_learner<#>.json
                //with learner name + params so they can be reconstructed
                //and some metadata (what domain they were trained in, how many observations they trained on, ...)
                //std::string str_timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                //std::string str_pid = std::to_string(GetCurrentProcessId());
                //std::string learner_filename_prefix = "model_" + str_timestamp + "_" + str_pid + "_learner";
                json learner_data{
                    {"name", learner_data_prev.at("name")},
                    {"parameters", learner_data_prev.at("parameters")},
                    {"domain", {
                        {"name", domain_name},
                        {"parameters", domain_parameters}
                    }},
                    {"observations", (observation_count - 1) + learner_data_prev.at("observations").get<int>()},
                    {"model", learner->to_json()}
                };
                //
                std::string str_id = std::to_string(i);
                std::string model_filename = (k == 1) ? (file_models + "_" + str_id + ".json") : (file_models + "_" + str_id + "_" + std::to_string(index) + ".json");
                std::ofstream output_learner(model_filename);
                if (!output_learner.good()) {
                    Logger::log(Logger::formatString("Failed to open model output file \"%s\"", model_filename.c_str()), true);
                    return EXIT_FAILURE;
                }
                output_learner << std::setw(2) << learner_data << std::endl;
                output_learner.close();
            }
            delete learner;
        }

        //
        delete env;
        //
        input.close();
        output.close();

        //
        if (k > 1) progress_k.update(index + 1);
    }
    if (k > 1) progress_k.end();
    Logger::indent_pop();
}

//<learner file(s)> <model file name stem> <data output file> <observations file> [learning enabled, true|false, default=false] [k=1]
int run_predict_pt(int argc, char** argv) {
    //<learner file(s)> <model file name stem> <data output file> <observations file> [learning enabled, true|false, default=false] [k=1]

    //check args
    if (argc < 4) {
        Logger::log("predict_pt needs 4-6 arguments: <learner file(s)> <model file name stem> <data output file> <observations file> [learning enabled, true|false, default=false] [k=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    std::string learner_list = argv[0];
    std::string file_models = argv[1];
    std::string file_output = argv[2];
    std::string file_input = argv[3];
    bool learning_enabled = false;
    if (argc > 4) learning_enabled = (std::string(argv[4]) == "true");
    int k = 1;
    if (argc > 5) k = atoi(argv[5]);
    //
    return run_predict_pt(learner_list, file_models, file_output, file_input, learning_enabled, k);
}

int run_avg(const std::string& file_out, const std::string& file_in, int k, int n) {
    //TODO this is a tmp fix for the max file open limit (default=512)
    //gotta figure out a better way to do this, that hopefully still lets me stream the input data
    //e.g. maybe break up the k files into groups of 100 or something
    _setmaxstdio(k + 10);
    //
    int n_learners = -1; //will figure this out on first loop
    //output file
    std::ofstream output(file_out);
    if (!output.good()) {
        Logger::log(Logger::formatString("Failed to open output file \"%s\"", file_out.c_str()), true);
        return EXIT_FAILURE;
    }
    //input files
    std::vector<std::ifstream> files_in;
    for (int i = 0; i < k; i++) {
        std::string filename_in = (k == 1) ? file_in : (file_in + "_" + std::to_string(i) + ".txt");
        
        //find how many learners there are
        if (i == 0) {
            std::ifstream input_tmp(filename_in);
            if (!input_tmp.good()) {
                Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
                return EXIT_FAILURE;
            }
            std::string line;
            getline(input_tmp, line);
            std::vector<std::string> line_split = str_split(line, "\t");
            n_learners = line_split.size() - 1; //first column is observation #
            input_tmp.close();
        }

        //attempt to open the files
        files_in.emplace_back(filename_in);
        std::ifstream& input = files_in.back();
        if (!input.good()) {
            Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename_in.c_str()), true);
            return EXIT_FAILURE;
        }
    }
    //run
    int observation_number = 0;
    int current_count = 0; //count up to n, then write
    std::vector<double> errors(n_learners, 0.0); //avg error for each learner (avg'd over the k files), summed up to n times
    //
    std::vector<std::string> lines(k);
    while (true) {
        std::vector<double> sum(n_learners, 0.0); //holds total learner error over the k files for this single row
        //get line from each input file
        bool do_break = false;
        for (int i = 0; i < k; i++) {
            std::ifstream& input = files_in[i];
            std::string& line = lines[i];
            getline(input, line);
            if (line == "") do_break = true;
            else {
                //parse line
                std::vector<std::string> line_split = str_split(line, "\t");
                for (int j = 0; j < n_learners; j++) {
                    double val = std::stof(line_split[j + 1]); //extract error value of this learner
                    sum[j] += val;
                }
            }
        }
        if (do_break) break; //got a blank line, we're done
        //store avgs
        for (int i = 0; i < n_learners; i++) {
            errors[i] += sum[i] / k;
        }
        //
        current_count++;
        observation_number++;
        //output if counted to n
        if (current_count == n) {
            output << observation_number;
            for (int i = 0; i < n_learners; i++) {
                output << '\t' << (errors[i] / n); //avg value over the n steps
                errors[i] = 0.0; //reset accumulated error
            }
            output << '\n';
            current_count = 0;
        }
    }
    //cleanup
    output.close();
    for (int i = 0; i < k; i++) {
        files_in[i].close();
    }
    //
    return EXIT_SUCCESS;
}

//avg <output file name> <data file> [k=1] [n=1]
int run_avg(int argc, char** argv) {
    //check args
    if (argc < 2) {
        Logger::log("avg needs 2-4 arguments: <output file> <data file> [k=1] [n=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    std::string file_out = argv[0];
    std::string file_in = argv[1];
    int k = 1;
    if (argc > 2) k = atoi(argv[2]);
    int n = 1;
    if (argc > 3) n = atoi(argv[3]);
    //
    return run_avg(file_out, file_in, k, n);
}

int run_print(int argc, char** argv) {
    //basically just load the model and then call print()

    //check arg
    if (argc < 1) {
        Logger::log("print needs 1 argument: learner file", true);
        return EXIT_FAILURE;
    }
    //get arg
    std::string filename = argv[0];
    //attempt to open the file
    std::ifstream input(filename);
    if (!input.good()) {
        Logger::log(Logger::formatString("Failed to open input file \"%s\"", filename.c_str()), true);
        return EXIT_FAILURE;
    }
    //read the learner data
    json learner_data;
    input >> learner_data;

    //re-construct the domain
    Environment* env = nullptr;
    {
        json& domain_data = learner_data["domain"];
        const std::string& domain_name = domain_data["name"].get<std::string>();
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        const json& domain_parameters = domain_data["parameters"];
        env = constructor.constructor(domain_parameters.get<std::map<std::string, std::string>>());

        //print some info for the user
        //domain + params
        printf("Domain: %s\n", domain_name.c_str());
        for (const auto& el : domain_parameters.items()) {
            printf(" %s: %s\n", el.key().c_str(), el.value().get<std::string>().c_str());
        }
    }
    Types& types = env->getTypes();

    //construct the learner
    Learner* learner = nullptr;
    {
        std::string learner_name = learner_data["name"].get<std::string>();
        auto it = CONTENTS.learners.find(learner_name);
        if (it == CONTENTS.learners.end()) {
            Logger::log(Logger::formatString("Learner not found: \"%s\"", learner_name.c_str()), true);
            return EXIT_FAILURE;
        }
        LearnerConstructor& constructor = it->second;
        const json& learner_parameters = learner_data["parameters"];
        learner = constructor.constructor(env, learner_parameters.get<std::map<std::string, std::string>>());
        learner->from_json(learner_data["model"]);

        //print some info for the user
        //learner + params
        printf("Learner: %s\n", learner_name.c_str());
        for (const auto& el : learner_parameters.items()) {
            printf(" %s: %s\n", el.key().c_str(), el.value().get<std::string>().c_str());
        }
        printf("Observations: %d\n", learner_data["observations"].get<int>());
    }

    printf("\n");

    //print
    learner->print(stdout);

    //
    delete learner;
    delete env;
    //
    return EXIT_SUCCESS;
}

//handle ctrl-c to stop running test mode
bool running_test_loop = true;
BOOL WINAPI test_ctrlc_handler(DWORD dwCtrlType)
{
    running_test_loop = false;
    return TRUE;
}
//test <learner> <domain> <m>
int run_test(int argc, char** argv) {

    //check args
    if (argc < 3) {
        Logger::log("test needs 3 arguments: learner, domain, m, [m2 = 2]", true);
        return EXIT_FAILURE;
    }

    //get args
    std::string learner_str = argv[0]; //learner(args)
    std::string domain_str = argv[1]; //domain(args)
    int m = atoi(argv[2]); //number of actions to generate per start state
    int m2 = 1; //environments per print cycle
    if (argc >= 4) m2 = atoi(argv[3]);

    //create domain
    auto domain_nameargs = str_args(domain_str);
    std::string domain_name = domain_nameargs.first;
    std::string domain_args = domain_nameargs.second;
    Environment* env = nullptr;
    {
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        std::map<std::string, std::string> args;
        if (!constructor.params.parse(domain_args, args)) {
            return EXIT_FAILURE;
        }
        env = constructor.constructor(args);
    }
    Types& types = env->getTypes();
    Random random;
    random.seed_time();

    //create learner
    auto learner_nameargs = str_args(learner_str);
    std::string learner_name = learner_nameargs.first;
    std::string learner_args = learner_nameargs.second;
    //
    Learner* learner = nullptr;
    {
        auto it = CONTENTS.learners.find(learner_name);
        if (it == CONTENTS.learners.end()) {
            Logger::log(Logger::formatString("Learner not found: \"%s\"", learner_name.c_str()), true);
            return EXIT_FAILURE;
        }
        LearnerConstructor& constructor = it->second;
        std::map<std::string, std::string> args;
        if (!constructor.params.parse(learner_args, args)) {
            return EXIT_FAILURE;
        }
        learner = constructor.constructor(env, args);
    }

    //run
    int batches = 0; //number of batches (starting level + m actions) completed
    int observations = 0;
    int observed_last_error = 0;
    SetConsoleCtrlHandler(test_ctrlc_handler, true);
    while (running_test_loop) {
        //inner loop
        while (running_test_loop) {
            Progress progress(Logger::formatString("Batch %d", batches + 1), m * m2);
            int step = 0;
            double total_error = 0; //total error this batch
            for (int i = 0; i < m2; i++) {
                //initial state, error tracking
                State state = env->createRandomState(random);
                State sHidden = env->hideInformation(state);
                //env->print(state);
                //loop
                for (int j = 0; j < m; j++) {
                    //predict+observe
                    ActionId action = random.sample(types.getActions()).id;
                    StateDistribution nextStates = env->act(state, action, random);
                    //
                    StateDistribution predictedStates = learner->predictTransition(sHidden, action, random);
                    double error = nextStates.error(predictedStates);
                    //
                    total_error += error;
                    State nextState = nextStates.sample(random);
                    State nextHidden = env->hideInformation(nextState);
                    learner->observeTransition(sHidden, action, nextHidden);
                    //
                    progress.update(++step);
                    //
                    observations++;
                    if (error > 0) observed_last_error = observations;
                    //
                    state = nextState;
                    sHidden = nextHidden;
                }
            }
            progress.end();
            batches++;
            //print error stuff
            printf("Batch %d, observations %d, avg error %05.3f, last error @ %d\n", batches, observations, double(total_error) / (m * m2), observed_last_error);
        }
        //user hit ctrl-c, print model and ask if they want to exit

        learner->print(stdout);

        printf("\n\nContinue? y/n\n");
        std::string line;
        while (line.size() != 1 && std::cin.good()) {
            std::getline(std::cin, line);
            printf("Input: \"%s\"\n", line.c_str());
        }
        char c = line.at(0);
        if (c == 'y') {
            running_test_loop = true;
            printf("Continuing...\n");
        }
        else {
            printf("Exiting...\n");
        }

        //running_test_loop = true;
    }
    SetConsoleCtrlHandler(test_ctrlc_handler, false);
    printf("Exited test loop\n");

    //
    delete learner;
    delete env;
    //
    return EXIT_SUCCESS;
}

//exec <n> <m> <k> <domain> <stem> <learner(s)> [n_avg=1]
int run_exec(int argc, char** argv) {
    //check args
    if (argc < 6) {
        Logger::log("exec needs 6-7 arguments: <n> <m> <k> <domain> <stem> <learner(s)> [n_avg=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    int n = atoi(argv[0]); //number of levels (start states) to generate
    int m = atoi(argv[1]); //number of actions to generate per start state
    int k = atoi(argv[2]); //number of sequences to run
    std::string domain_nameargs = argv[3]; //domain(args)
    std::string filestem = argv[4];
    std::string learner_list = argv[5];
    int n_avg = 1;
    if (argc > 6) {
        n_avg = atoi(argv[6]);
    }
    //
    std::string file_levels = "levels/" + filestem;
    std::string file_models = "models/" + filestem;
    std::string file_data = "data/" + filestem;
    std::string file_avg = "data/avg_" + filestem + ".txt";

    //gen n m domain levels/stem k
    int status = EXIT_SUCCESS;
    if ((status = run_gen(n, m, domain_nameargs, file_levels, k)) != EXIT_SUCCESS) {
        return status;
    }
    //predict learners models/stem data/stem levels/stem k
    if ((status = run_predict(learner_list, file_models, file_data, file_levels, k)) != EXIT_SUCCESS) {
        return status;
    }
    //avg data/avg_stem.txt data/stem k
    if ((status = run_avg(file_avg, file_data, k, n_avg)) != EXIT_SUCCESS) {
        return status;
    }
    //
    return status;
}

//exec_t <n> <m> <k> <domain> <stem> <n2> <m2> <domain2> <stem2> <learner(s)> [learning=false] [n_avg=1]
int run_exec_t(int argc, char** argv) {
    //check args
    if (argc < 10) {
        Logger::log("exec_t needs 10-12 arguments: <n> <m> <k> <domain> <stem> <n2> <m2> <domain2> <stem2> <learner(s)> [learning=false] [n_avg=1]", true);
        return EXIT_FAILURE;
    }
    //get args
    //initial training
    int n = atoi(argv[0]); //number of levels (start states) to generate (for initial training)
    int m = atoi(argv[1]); //number of actions to generate per start state (for initial training)
    int k = atoi(argv[2]); //number of sequences to run
    std::string domain_nameargs = argv[3]; //domain(args)
    std::string filestem = argv[4];
    //transfer
    int n2 = atoi(argv[5]); //number of levels (start states) to generate
    int m2 = atoi(argv[6]); //number of actions to generate per start state
    std::string domain_nameargs2 = argv[7]; //domain(args)
    std::string filestem2 = argv[8];
    //
    std::string learner_list = argv[9];
    //
    bool learning_enabled = false;
    if (argc > 10) learning_enabled = (std::string(argv[10]) == "true");
    int n_avg = 1;
    if (argc > 11) {
        n_avg = atoi(argv[11]);
    }
    //
    std::string file_levels = "levels/" + filestem;
    std::string file_models = "models/" + filestem;
    std::string file_data = "data/" + filestem;
    std::string file_avg = "data/avg_" + filestem + ".txt";
    std::string suffix2 = learning_enabled ? "tl" : "t";
    std::string file_levels2 = "levels/" + filestem2;
    std::string file_models2 = "models/" + filestem2 + "_" + suffix2;
    std::string file_data2 = "data/" + filestem2 + "_" + suffix2;
    std::string file_avg2 = "data/avg_" + filestem2 + "_" + suffix2 + ".txt";
    std::string learner_list2;
    int n_learners = str_split(learner_list, ";").size();
    for (int i = 0; i < n_learners; i++) {
        learner_list2 += (file_models + "_" + std::to_string(i));
        if (i < n_learners - 1) {
            learner_list2 += ";";
        }
    }

    int status = EXIT_SUCCESS;
    //gen n m domain levels/stem k
    if ((status = run_gen(n, m, domain_nameargs, file_levels, k)) != EXIT_SUCCESS) {
        return status;
    }
    //predict learners models/stem data/stem levels/stem k
    if ((status = run_predict(learner_list, file_models, file_data, file_levels, k)) != EXIT_SUCCESS) {
        return status;
    }
    //avg data/avg_stem.txt data/stem k n_avg
    if ((status = run_avg(file_avg, file_data, k, n_avg)) != EXIT_SUCCESS) {
        return status;
    }
    //gen n2 m2 domain2 levels/stem2 k
    if ((status = run_gen(n2, m2, domain_nameargs2, file_levels2, k)) != EXIT_SUCCESS) {
        return status;
    }
    //predict_pt models/stem_0... models/stem2_t(l) data/stem2_t(l) levels/stem2 <learning?> k
    if ((status = run_predict_pt(learner_list2, file_models2, file_data2, file_levels2, learning_enabled, k)) != EXIT_SUCCESS) {
        return status;
    }
    //avg data/avg_stem2_t(l).txt data/stem2_t(l) k
    if ((status = run_avg(file_avg2, file_data2, k, n_avg)) != EXIT_SUCCESS) {
        return status;
    }
    //
    return status;
}

int run_bfs_to(const State& state_start, const State& state_end, Learner* learner, Environment* env, Random& random) {
    Types& types = env->getTypes();
    std::vector<Action> actions = types.getActions();

    int steps_taken = 0;
    int nodes_evaluated = 0;

    //BFS loop
    State state_current = state_start;
    int plan_length_limit = 30; //max depth to run BFS before early termination of planning
    while (!(state_current == state_end)) {
        printf(" Running planning iteration...\n");
        //
        typedef std::pair<int, State> P;
        std::priority_queue<P, std::vector<P>, std::greater<P>> frontier;
        std::set<State> visited;
        std::map<State, std::pair<State, Action>> sources; //[state] -> [source state], for backtracking path creation
        std::map<State, int> depths; //[state] -> distance from state_current at beginning of the loop
        frontier.push({ state_current.diff(state_end).length() , state_current});
        visited.insert(state_current);
        depths[state_current] = 0;
        //run BFS
        printf("  Starting A*...\n");
        bool solution_found = false;
        bool hit_limit = false;
        State state_last;
        while (frontier.size()) {
            P p = frontier.top();
            state_last = p.second;
            nodes_evaluated++;
            //check if reached goal
            if (state_last == state_end) {
                printf("   Found goal!\n");
                solution_found = true;
                break;
            }
            //
            frontier.pop();
            //check if reached depth limit
            if (depths.at(state_last) >= plan_length_limit) {
                //printf("  Reached planning depth limit\n");
                hit_limit = true;
                break;
            }
            //get neighbors for each possible action
            //shuffle order of actions so naive agent tends to take random paths
            random.shuffle(actions);
            for (const Action& action : actions) {
                StateDistribution predict = learner->predictTransition(state_last, action.id, random);
                State next = predict.sample(random);
                //
                if (visited.insert(next).second) {
                    //new state
                    int depth = depths[state_last] + 1;
                    frontier.push({ depth + next.diff(state_end).length(), next });
                    sources[next] = { state_last, action };
                    depths[next] = depth;
                }
            }
        }
        if (!solution_found) {
            if (hit_limit) {
                printf("   Reached planning depth limit, increasing by 1\n");
                plan_length_limit++;
                //model couldn't find path to goal within depth limit
                //which means (1) either goal is farther than limit, or (2) model can't predict well enough
            }
            else {
                printf("   Goal unreachable? Taking random action\n");
                //model couldn't find path to goal
                //which means it hasn't learned enough yet

                //clear the path that was found
                //so a random action will be taken to gain training data
                sources.clear();
            }
        }
        //printf(" Constructing plan...\n");
        //execute search to whatever "state_current" is, then possibly try again
        std::vector<Action> plan;
        //construct plan in reverse order
        State state_next = state_last;
        while (true) {
            if (sources.find(state_next) == sources.end()) break; //reached beginning
            auto& pair = sources.at(state_next);
            state_next = pair.first;
            plan.push_back(pair.second);
        }
        //reverse the plan
        std::reverse(plan.begin(), plan.end());
        //ensure plan has at least 1 element
        if (plan.empty()) {
            plan.push_back(random.sample(types.getActions()));
        }
        //execute the plan
        //printf(" Executing plan...\n");
        //printf("  Current state:\n\n");
        //env->print(state_current);
        //printf("\n\n");
        for (const Action& action : plan) {
            State s_hidden = env->hideInformation(state_current);
            State state_next = env->act(state_current, action.id, random).sample(random);
            State sp_hidden = env->hideInformation(state_next);


            StateDistribution predict = learner->predictTransition(s_hidden, action.id, random);

            steps_taken++;

            //train the agent while planning
            learner->observeTransition(s_hidden, action.id, sp_hidden);

            //printf("  Action: %s\n", action.name.c_str());
            //printf("  Result:\n\n");
            //env->print(state_next);
            //printf("\n\n");
            state_current = state_next;

            if (state_current == state_end) break;

            //Sleep(1000);
        }
        printf("  Done with planning iteration\n");
    }
    printf(" Reached goal state in %d steps, evaluated %d states\n", steps_taken, nodes_evaluated);
    //Sleep(1000);


    return steps_taken;
}

int run_plan(const std::string& learner_str, const std::string& domain_str, int n_train, int m_train) {
    /*
    currently:
    0. construct domain/learner
    0A. train the learner on n levels, m steps each
    1. generate a random starting state (A)
    2. generate a random reachable goal state (B)
    3. use the learner's model to do a BFS search and construct a plan to get from A -> B
    4. execute the sequence of actions in the plan
    5. if goal state is not reached, goto 3
    */

    int goal_length_limit = 100; //number of moves to use when generating goal state
    //int plan_length_limit = 10; //max depth to run BFS before early termination of planning

    //create domain
    auto domain_nameargs = str_args(domain_str);
    std::string domain_name = domain_nameargs.first;
    std::string domain_args = domain_nameargs.second;
    Environment* env = nullptr;
    {
        auto it = CONTENTS.domains.find(domain_name);
        if (it == CONTENTS.domains.end()) {
            Logger::log(Logger::formatString("Domain not found: \"%s\"", domain_name.c_str()), true);
            return EXIT_FAILURE;
        }
        DomainConstructor& constructor = it->second;
        std::map<std::string, std::string> args;
        if (!constructor.params.parse(domain_args, args)) {
            return EXIT_FAILURE;
        }
        env = constructor.constructor(args);
    }
    Types& types = env->getTypes();
    Random random;
    random.seed_time();

    //create learner
    auto learner_nameargs = str_args(learner_str);
    std::string learner_name = learner_nameargs.first;
    std::string learner_args = learner_nameargs.second;
    //
    Learner* learner = nullptr;
    {
        auto it = CONTENTS.learners.find(learner_name);
        if (it == CONTENTS.learners.end()) {
            Logger::log(Logger::formatString("Learner not found: \"%s\"", learner_name.c_str()), true);
            return EXIT_FAILURE;
        }
        LearnerConstructor& constructor = it->second;
        std::map<std::string, std::string> args;
        if (!constructor.params.parse(learner_args, args)) {
            return EXIT_FAILURE;
        }
        learner = constructor.constructor(env, args);
    }

    //train the learner
    printf("Training...\n");
    Progress progress("Training", n_train * m_train);
    int training_step = 0;
    for (int i = 0; i < n_train; i++) {
        //generate a level
        State state = env->createRandomState(random);
        //generate m observations
        for (int j = 0; j < m_train; j++) {
            //generate an action
            const Action& a = random.sample(types.getActions());
            State next = env->act(state, a.id, random).sample(random);
            //
            learner->observeTransition(state, a.id, next);
            //
            state = next;
            //
            training_step++;
            progress.update(training_step);
        }
    }
    progress.end();

    printf("Planning...\n");
    Learner* learner2 = new Oracle(env); //baseline

    while (true) {
        printf("Press enter to test model, ctrl-c to exit\n");
        getchar();


        //
        State state_start = env->createRandomState(random);
        State state_end = env->createRandomGoalState(state_start, goal_length_limit, random);
        //

        printf("\nStart state:\n");
        env->print(state_start);
        printf("\nGoal state:\n");
        env->print(state_end);
        printf("\n\n");

        printf("Press enter to plan...\n");
        //getchar();

        int steps_learner = run_bfs_to(state_start, state_end, learner, env, random);
        int steps_oracle = run_bfs_to(state_start, state_end, learner2, env, random);

        printf("Path steps: %d, %d\n", steps_learner, steps_oracle);
    }
}

//plan <learner> <domain>
int run_plan(int argc, char** argv) {
    //check args
    if (argc < 4) {
        Logger::log("plan needs 4 arguments: <learner> <domain> <n> <m>", true);
        return EXIT_FAILURE;
    }
    //get args
    std::string learner = argv[0];
    std::string domain = argv[1];
    int n = atoi(argv[2]);
    int m = atoi(argv[3]);
    //
    return run_plan(learner, domain, n, m);
}

////////////////////////////////////////////////////////////////////////////////
//main entrypoint
////////////////////////////////////////////////////////////////////////////////

void init_contents() {
    ////////////////////////////////////////////////////////////////////////////////
    //domains
    ////////////////////////////////////////////////////////////////////////////////
    CONTENTS.addDomain("test",
        Parameters(),
        [](const std::map<std::string, std::string>& params) {
            return new DomainTest();
        }
    );
    CONTENTS.addDomain("lights",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("min", 2, 1))
        .addParameter(Parameter::createIntParamDefault("max", 10, 0))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainLights(std::stoi(params.at("min")), std::stoi(params.at("max")));
        }
    );
    CONTENTS.addDomain("walls",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainWalls(std::stoi(params.at("width")), std::stoi(params.at("height")));
        }
    );
    CONTENTS.addDomain("paths",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("n", 2, 1)) //how many dimensions the space will be
        .addParameter(Parameter::createIntParamDefault("b", 100, 1)) //how many path blocks to place
        .addParameter(Parameter::createIntParamDefault("a", 0, 0)) //how many actions to enable (0 = all)
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainPaths(std::stoi(params.at("n")), std::stoi(params.at("b")), std::stoi(params.at("a")));
        }
    );
    CONTENTS.addDomain("players",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        .addParameter(Parameter::createIntParamDefault("n", 2, 1, 26))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainPlayers(std::stoi(params.at("width")), std::stoi(params.at("height")), std::stoi(params.at("n")));
        }
    );
    CONTENTS.addDomain("moves",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        .addParameter(Parameter::createIntParamDefault("n", 1, 1))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainMoves(std::stoi(params.at("width")), std::stoi(params.at("height")), std::stoi(params.at("n")));
        }
    );
    CONTENTS.addDomain("enigma",
        Parameters()
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainEnigma();
        }
    );
    CONTENTS.addDomain("complex",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        .addParameter(Parameter::createIntParamDefault("mode", 0, 0, 5))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainComplex(std::stoi(params.at("width")), std::stoi(params.at("height")), std::stoi(params.at("mode")));
        }
    );
    CONTENTS.addDomain("fish",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        .addParameter(Parameter::createIntParamDefault("fishes", 1, 1))
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainFish(std::stoi(params.at("width")), std::stoi(params.at("height")), std::stoi(params.at("fishes")));
        }
        );
    CONTENTS.addDomain("doors",
        Parameters()
        .addParameter(Parameter::createIntParamDefault("width", 8, 5))
        .addParameter(Parameter::createIntParamDefault("height", 8, 5))
        .addParameter(Parameter::createIntParamDefault("doors", 4, 0))
        .addParameter(Parameter::createIntParamDefault("colors", 2, 1, 2)) //number of colors of doors to generate; player still can always switch 0-1
        ,
        [](const std::map<std::string, std::string>& params) {
            return new DomainWallsDoors(std::stoi(params.at("width")), std::stoi(params.at("height")), std::stoi(params.at("doors")), std::stoi(params.at("colors")));
        }
        );

    ////////////////////////////////////////////////////////////////////////////////
    //learners
    ////////////////////////////////////////////////////////////////////////////////
    CONTENTS.addLearner("oracle",
        Parameters(),
        [](const Environment* env, const std::map<std::string, std::string>& params) {
            return new Oracle(env);
        }
    );
    CONTENTS.addLearner("qora",
        Parameters()
        .addParameter(Parameter::createFloatParamDefault("alpha", 0.05, 0, 1))
        ,
        [](const Environment* env, const std::map<std::string, std::string>& params) {
            return new l_qora::LearnerQORA(env->getTypes(), std::stod(params.at("alpha")));
        }
    );
}

int main(int argc, char** argv)
{
    //initialize log
    Logger::init("blockworld");
    std::string exe_running = "Running:";
    for (int i = 0; i < argc; i++) {
        exe_running += std::string(" \"") + argv[i] + std::string(1, '\"');
    }
    Logger::log(exe_running);
    //initialize contents (domains, learners)
    init_contents();
    //if no args are given, print usage and quit
    if (argc == 0) {
        //how did this even happen?
        print_usage("QORA.exe");
        Logger::quit();
        return EXIT_SUCCESS;
    }
    if (argc == 1) {
        print_usage(argv[0]);
        Logger::quit();
        return EXIT_SUCCESS;
    }
    //if args are given, identify which mode is specified and run it
    std::string exe = argv[0];
    std::string mode = argv[1];
    //help?
    if (mode == "help") {
        print_usage(exe.c_str());
        return EXIT_SUCCESS;
    }
    //skip the executable+mode
    argc -= 2;
    argv += 2;
    //
    typedef int(*runnable)(int, char**);
    std::map<std::string, runnable> modes{
        {"play", run_play},
        {"types", run_types},
        {"gen", run_gen},
        {"verbose", run_verbose},
        {"flatten", run_flatten},
        {"view", run_view},
        {"predict", run_predict},
        {"predict_pt", run_predict_pt},
        {"avg", run_avg},
        {"print", run_print},
        {"test", run_test},
        {"exec", run_exec},
        {"exec_t", run_exec_t},
        {"plan", run_plan}
    };
    auto it = modes.find(mode);
    if (it == modes.end()) {
        Logger::log(Logger::formatString("Mode not found: \"%s\"\n", mode.c_str()), true);
        print_usage(exe.c_str());
        Logger::quit();
        return EXIT_FAILURE;
    }
    else {
        int status = it->second(argc, argv);
        Logger::quit();
        return status;
    }
}