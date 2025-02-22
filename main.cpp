#include <iostream>
#include <string>
#include <random>
#include <thread>
#include <memory>
#include <future>
#include <map>
#include <algorithm>
#include <format>

std::vector<std::string> first_name = {"Vlad", "Julius", "Archemedes", "Cragg", "Leonardo", "Marcellus", "Dante",
"Romulus", "Giorgio", "Hector" };
std::vector<std::string> title = {"the Impaler", "the Wise", "the Furious", "the Strong", "the Large", "the Small", "the Savage", 
"the Adorned", "the Great", "the Imposing"};

// Function template for getting player input
template <typename T>
T getPlayerInput(const std::string& prompt) {
    T input;
    std::cout << prompt;
    std::cin >> input;
    return input;
}

std::string generate_opponent_name(){
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(0, first_name.size());
    std::string name;
    name = first_name[distrib(gen)] + " " + title[distrib(gen)];
    return name;

}

// Base Player class
class Player {
public:
    Player(const std::string& name) : name_(name) {}
    virtual std::string getMove() = 0;
    std::string getName() const { return name_; }
    virtual ~Player() = default;

protected:
    std::string name_;
};

// Human Player class: inherit from Player Class
class HumanPlayer : public Player {
public:
    HumanPlayer(const std::string& name) : Player(name) {wins=0;}
    int wins;
    int loss;
    bool alive = true;
    std::string getMove() override {
        std::string move;
        while (true) {
            move = getPlayerInput<std::string>("Enter your move (rock, paper, scissors): ");
            std::transform(move.begin(), move.end(), move.begin(), ::tolower);
            if (move == "rock" || move == "paper" || move == "scissors") {
                return move;
            } else {
                std::cout << "Invalid move. Try again.\n";
            }
        }
    }
    void increment_wins(){
        wins++;
    }
    void increment_loss(){
        loss++;
    }
};

// Computer Player class: Inherit from Player
class ComputerPlayer : public Player {
public:
    ComputerPlayer(const std::string& name) : Player(name) {}
    std::string getMove() override {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> distrib(0, 2);

        std::vector<std::string> moves = {"rock", "paper", "scissors"};
        return moves[distrib(gen)];
    }
};

// Game class
class Game {
public:
    Game(std::unique_ptr<HumanPlayer> player1, std::unique_ptr<ComputerPlayer> player2)
        : player1_(std::move(player1)), player2_(std::move(player2)) {}

    std::unique_ptr<HumanPlayer> play() {
        // run human/computer on seperate threads
        auto player1MoveFuture = std::async(std::launch::async, [&]() { return player1_->getMove(); });
        auto player2MoveFuture = std::async(std::launch::async, [&]() { return player2_->getMove(); });
        // get values
        std::string player1Move = player1MoveFuture.get();
        std::string player2Move = player2MoveFuture.get();

        std::cout << player1_->getName() << " chose: " << player1Move << std::endl;
        std::cout << player2_->getName() << " chose: " << player2Move << std::endl;

        determineWinner(player1Move, player2Move);
        return std::move(player1_);
    }

private:
    void determineWinner(const std::string& move1, const std::string& move2) {
        std::map<std::pair<std::string, std::string>, int> results = {
            {{"rock", "scissors"}, 1},
            {{"paper", "rock"}, 1},
            {{"scissors", "paper"}, 1},
            {{"scissors", "rock"}, -1},
            {{"rock", "paper"}, -1},
            {{"paper", "scissors"}, -1},
            {{"rock", "rock"}, 0},
            {{"paper", "paper"}, 0},
            {{"scissors", "scissors"}, 0}};

        int result = results[{move1, move2}];

        if (result == 1) {
            std::cout << player1_->getName() << " wins!\n";
            player1_->increment_wins();
        } else if (result == -1) {
            std::cout << player2_->getName() << " wins!\n";
            player1_->increment_loss();
        } else {
            std::cout << "It's a tie!\n";
        }
    }

    std::unique_ptr<HumanPlayer> player1_;
    std::unique_ptr<ComputerPlayer> player2_;
};

void clear_screen(){
    std::cout << "\033[2J\033[1;1H";
}

// display start game message and allows user to change the settings
std::array<int,2> start_game(const std::string& playername, std::array<int,2> settings){
    bool custom_settings = false;
    std::cout << std::format("Greetings {}! You are about to embark on a great challenge."
    " You need to win {} out of {} matches in order to go down in history as a great gladiator\n",
     playername, settings[1], settings[0]);
    std::string continue_string = getPlayerInput<std::string>("Would you like to edit these settings (y/n)?");
    if (continue_string=="y"){
        bool valid = false;
        while (!valid){
            settings[0] = getPlayerInput<int>("Desired Number of Rounds?[1-9]");
            settings[1] = getPlayerInput<int>("Target Number of Wins?[1-9]");
            if (settings[0] >= settings[1]){valid = true;}
            if (settings[0] < settings[1]){std::cout << "Number of rounds must be greater than target wins" << std::endl;}

        };
    }
    return settings;
}

int main() {
    // default settings
    int round = 1;
    int target_round = 5;
    int target_wins = 2;

    std::array<int, 2> settings = {target_round, target_wins};

    std::string playerName = getPlayerInput<std::string>("Enter your name: ");
    auto human = std::make_unique<HumanPlayer>(playerName);

    settings = start_game(playerName, settings);

    while (round <= settings[0]) {
        if (round > 1){
            std::string prompt = std::format("Would you like to continue (y/n). You are only {} wins away from greatness!", (settings[1]-human->wins));
            std::string continue_string = getPlayerInput<std::string>(prompt);
            if (continue_string=="n") {break;}
        }

        std::string opponent_name = generate_opponent_name();
        auto enemy = std::make_unique<ComputerPlayer>(opponent_name);
        clear_screen();
        std::cout << "\n--- Round " << round << " ---" << std::endl;
        std::cout << "You are facing: " << opponent_name << std::endl;

        // by using uniqueptr the enemy object is deleted after every 
        // iteration. 
        // human player is passed back so that wins can be tracked
        Game game(std::move(human), std::move(enemy));
        human = game.play();
    
        std::cout << "Player Wins: " << human->wins << std::endl;
        round++;
    }

    return 0;
}