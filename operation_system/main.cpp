#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <iterator>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>

// Базовий клас стратегії для обчислень
class CalculationStrategy {
public:
    virtual double calculate(int x) const = 0;
    virtual ~CalculationStrategy() = default;
};

// Конкретні стратегії
class FunctionG : public CalculationStrategy {
public:
    double calculate(int x) const override { return x * x * 1.5; }
};

class FunctionH : public CalculationStrategy {
public:
    double calculate(int x) const override { return std::sqrt(x); }
};

class FunctionF : public CalculationStrategy {
public:
    double calculate(int x) const override { return x * x * x; }
};

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "SIGINT received, terminating program...\n";
        exit(0);
    }
}

void sigchldHandler(int signal) {
    // Очищаємо дочірні процеси
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        
    }
}



// Клас Менеджера для керування групами і компонентами
class Manager {
public:
    void createGroup(const std::string& name) {
        if (currentGroup.empty()) {
            currentGroup = name;
            std::cout << "Group \"" << name << "\" created.\n";
        } else {
            std::cerr << "Group \"" << currentGroup << "\" already exists.\n";
        }
    }

    void addComponent(const std::string& name, std::shared_ptr<CalculationStrategy> strategy, int arg) {
        componentsToRun[name] = {strategy, arg};
        std::cout << "Component \"" << name << "\" added to the group.\n";
    }

    void run() {
        if (currentGroup.empty()) {
            std::cerr << "Please create a group first.\n";
            return;
        }

        std::cout << "Running group \"" << currentGroup << "\"...\n";
        for (const auto& [name, strategyArgPair] : componentsToRun) {
            const auto& [strategy, arg] = strategyArgPair;
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            }

            if (pid == 0) {
                close(pipe_fd[0]);
                double result = strategy->calculate(arg);
                write(pipe_fd[1], &result, sizeof(result));
                close(pipe_fd[1]);
                exit(0);
            } else {
                close(pipe_fd[1]);
                activeComponents[name] = pid;

                double result;
                if (read(pipe_fd[0], &result, sizeof(result)) > 0) {
                    results[name] = result;
                    std::cout << "Component \"" << name << "\" completed.\n";
                } else {
                    perror("read");
                }
                close(pipe_fd[0]);
            }
        }
        std::cout << "Execution completed.\n";
    }

    void status() const {
        std::cout << "Component status:\n";
        for (const auto& [name, pid] : activeComponents) {
            std::cout << "Component \"" << name << "\": running (PID: " << pid << ")\n";
        }
    }

    void summary() const {
        std::cout << "Component results:\n";
        for (const auto& [name, result] : results) {
            std::cout << "Component \"" << name << "\" result: " << result << "\n";
        }
    }

    void clear() {
        for (const auto& [name, pid] : activeComponents) {
            kill(pid, SIGKILL);
        }
        activeComponents.clear();
        componentsToRun.clear();
        results.clear();
        currentGroup.clear();
        std::cout << "Group cleared.\n";
    }

private:
    std::map<std::string, std::pair<std::shared_ptr<CalculationStrategy>, int>> componentsToRun;
    std::map<std::string, pid_t> activeComponents;
    std::map<std::string, double> results;
    std::string currentGroup;
};

// Клас Команди для управління компонентами
class Command {
public:
    virtual void execute() = 0;
    virtual ~Command() = default;
};

// Конкретні команди
class CreateGroupCommand : public Command {
    Manager& manager;
    std::string groupName;
public:
    CreateGroupCommand(Manager& mgr, const std::string& name) : manager(mgr), groupName(name) {}
    void execute() override {
        manager.createGroup(groupName);
    }
};

class AddComponentCommand : public Command {
    Manager& manager;
    std::string componentName;
    std::shared_ptr<CalculationStrategy> strategy;
    int argument;
public:
    AddComponentCommand(Manager& mgr, const std::string& name, std::shared_ptr<CalculationStrategy> strat, int arg)
        : manager(mgr), componentName(name), strategy(strat), argument(arg) {}

    void execute() override {
        manager.addComponent(componentName, strategy, argument);
    }
};

class RunCommand : public Command {
    Manager& manager;
public:
    RunCommand(Manager& mgr) : manager(mgr) {}
    void execute() override {
        manager.run();
    }
};

// Клас Фабрики для створення компонентів
class ComponentFactory {
public:
    static std::shared_ptr<CalculationStrategy> createComponent(const std::string& type) {
        if (type == "g") return std::make_shared<FunctionG>();
        if (type == "h") return std::make_shared<FunctionH>();
        if (type == "f") return std::make_shared<FunctionF>();
        return nullptr;
    }
};



// Основна функція
int main() {
    signal(SIGINT, signalHandler);
    signal(SIGCHLD, sigchldHandler);

    Manager mgr;
    std::string input;

    std::cout << "Command line interface started. Enter \"help\" for command list.\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        // Розбиття введеного рядка на токени
        std::istringstream iss(input);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

        if (tokens.empty()) continue;

        // Обробка команд
        if (tokens[0] == "help") {
            std::cout << "Available commands:\n";
            std::cout << "  group <name>         - Create a new task group.\n";
            std::cout << "  new <component> <arg> - Add a component to the group.\n";
            std::cout << "  run                  - Run all components.\n";
            std::cout << "  status               - Check component statuses.\n";
            std::cout << "  summary              - Show results.\n";
            std::cout << "  clear                - Clear the group.\n";
            std::cout << "  exit                 - Exit the program.\n";
        } else if (tokens[0] == "group") {
            if (tokens.size() != 2) {
                std::cerr << "Usage: group <name>\n";
                continue;
            }
            CreateGroupCommand cmd(mgr, tokens[1]);
            cmd.execute();
        } else if (tokens[0] == "new") {
            if (tokens.size() != 3) {
                std::cerr << "Usage: new <component> <arg>\n";
                continue;
            }
            int arg = std::stoi(tokens[2]);
            auto strategy = ComponentFactory::createComponent(tokens[1]);
            if (strategy) {
                AddComponentCommand cmd(mgr, tokens[1], strategy, arg);
                cmd.execute();
            } else {
                std::cerr << "Unknown component: " << tokens[1] << "\n";
            }
        } else if (tokens[0] == "run") {
            RunCommand cmd(mgr);
            cmd.execute();
        } else if (tokens[0] == "status") {
            mgr.status();
        } else if (tokens[0] == "summary") {
            mgr.summary();
        } else if (tokens[0] == "clear") {
            mgr.clear();
        } else if (tokens[0] == "exit") {
            break;
        } else {
            std::cerr << "Unknown command: " << tokens[0] << "\n";
        }
    }

    return 0;
}
