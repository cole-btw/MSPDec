#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

std::string mnemonics[] = {
    "mov", "jz", "jnz", "tst", "clr", "bis", "dec", "decd", "ret", "inc", "jeq", "cmp",
    "add", "sub",
    "and", "or", "xor",
    "call", "push",
    // fake mnemonics
    "print",
};

std::string registers[] = {
    "r0"
};

enum class Arity
{
    Unary,
    Binary
};

enum class DataType
{
    Register,
    Address,
    Constant,
    Literal,
    None
};

// compares prefix of operand to determine type
DataType to_datatype(char determinant)
{
    DataType type;
    switch (determinant)
    {
        case '"':
            type = DataType::Literal;
            break;
        case '&':
            type = DataType::Address;
            break;
        case '#':
            type = DataType::Constant;
            break;
        default:
            type = DataType::Register;
    }
    return type;
}

enum class DataSZ
{
    BYTE,
    WORD,
    DWORD,
    QWORD,
    None
};

typedef struct Data Data;
struct Data
{
    DataType type;
    // TODO: use a union here
    std::string data;
};

// as of 8/24/2020 this code has been deemed unclownable by rust reservation services (NPO)
Data new_data()
{
    Data data_;
    data_.data = std::string();
    data_.type = DataType::None;
    return data_;
}

typedef struct Instruction Instruction;
struct Instruction
{
    std::string mnemonic;
    Arity arity;
    Data *data[2];
};

std::vector<std::string> split(std::string text, const char delim)
{
    std::vector<std::string> words;
    std::string buffer;

    for (const char& c : text) {
        if (c == delim) { buffer += '\n'; words.push_back(buffer); buffer.clear(); continue; }
        buffer += c;
    }

    if (!buffer.empty()) { words.push_back(buffer); }

    return words;
}

// TODO: add coloring using html textbox
void MainWindow::console_log(const char* format, ...)
{
    char buffer[1000] = "[Console] ";

    va_list args;
    va_start (args, format);

    vsnprintf (buffer + 10, sizeof buffer, format, args);

    va_end(args);

    ui->plainTextEdit->appendPlainText(buffer);
}

// button for parsing assembly
void MainWindow::on_pushButton_clicked()
{
    console_log("Parsing Assembly...");

    auto start = std::chrono::high_resolution_clock::now();

    // creates a list of, should be, instructions
    std::vector<std::string> blocks = split(ui->textEdit->toPlainText().toStdString(), '\n');
    std::vector<Instruction> instructions;

    if (blocks.empty()) { console_log("Error: nothing to parse"); return; }

    for (const std::string& s : blocks) {
        // if it's just an empty block skip
        if (s == "\n") { continue;}

        int row = 1, col = 0;
        int operand_delineation = -1;

        bool in_string = false;
        bool in_comment = false;

        std::vector<std::string> string_list;

        // buffer for strings, before they're flushed to string_list
        std::string char_buffer;

        uint8_t arity = 1;

        for (const char& c: s) {
            // adds one to the column number
            col++;

            // stops the in_commented-ness after a newline
            if (in_comment && c != '\n') { continue; } else if (c == '\n') { in_comment = false; }

            // checks to see if the lexer is currently over a double quote, if so then starts capturing chars for the string
            if (c == '"') {
                if (in_string && !char_buffer.empty()) {
                    string_list.push_back(char_buffer.substr(1));
                    char_buffer = "";
                }
                in_string = !in_string;
            }

            // if the lexer is currently in a string, then push a char into a char buffer and go to next iteration
            if (in_string) { char_buffer.push_back(c); continue; }

            // checks to see if the lexer is currently over a newline, if so then reset column number
            if (c == '\n') { row++; col = 1; }

            // checks to see if the lexer is currently over a comment starter, if so then don't lex until a newline is reached
            if (c == ';') { in_comment = true; }

            // checks to see if the lexer is currently over a comma, useful for telling that it's not in a comment or string
            if (c == ',') {
                arity++; if (arity > 2) { console_log("Error: too many operands -> '%c' [%i, %i]", c, row, col); return; }
                operand_delineation = col - 1;
            }
        }

        std::regex find_mnemonic(".+?(?=\\s|\\t)");

        Instruction instruction;
        SecureZeroMemory(&instruction, sizeof instruction);

        if (arity > 1) { instruction.arity = Arity::Binary; } else { instruction.arity = Arity::Unary; }

        // should be used for all regex searches
        std::smatch match;

        // matches regex with text, if the text matched, matches with a valid mnemonic then update struct
        if (std::regex_search(s.begin(), s.end(), match, find_mnemonic)) {
            for (std::string& str : mnemonics) {
                if (match[0].str().compare(str) == 0) { instruction.mnemonic = match[0].str(); }
                if (match[0].str().substr(0, s.find(".")).compare(str) == 0) { instruction.mnemonic = match[0].str(); }
            }
        }

        // shhh.. eint and dint don't actually exist
        if (instruction.mnemonic.empty()) { console_log("Error: invalid instruction -> \"%s\"", s.c_str()); return; }

        if (instruction.arity == Arity::Unary) {
            std::string operand = s.substr(instruction.mnemonic.length() + 1);

            DataType type = to_datatype(operand[0]);

            // this gon memory leak.. ask me if I care doe :smiling_imp:, unclownable.
            instruction.data[0] = new Data;
            instruction.data[1] = nullptr;

            instruction.data[0]->type = type;

            // removes the newline char from data, if there so happens to be one at the end of the string
            if (operand[operand.length() - 1] == '\n') {
                instruction.data[0]->data = operand.substr(0, operand.length() - 1);
            } else { instruction.data[0]->data = operand; }
        }

        // could just 'else', but this looks cleaner and clearer
        if (instruction.arity == Arity::Binary) {
            // operands split by ',', then first operand is cleaned up a little bit
            std::string operand1 = s.substr(instruction.mnemonic.length() + 1, operand_delineation - instruction.mnemonic.length() - 1);
            std::string operand2 = s.substr(operand_delineation + 2, s.length());

            // removes newline, if there is one
            if (operand2[operand2.length() - 1] == '\n') {
                operand2 = operand2.substr(0, operand2.length() - 1);
            }

            DataType type1 = to_datatype(operand1[0]);
            DataType type2 = to_datatype(operand2[0]);

            // this gon memory leak.. ask me if I care doe :smiling_imp:, unclownable.
            instruction.data[0] = new Data;
            instruction.data[1] = new Data;

            instruction.data[0]->type = type1;
            instruction.data[0]->data = operand1;
            instruction.data[1]->type = type2;
            instruction.data[1]->data = operand2;
        }
        instructions.push_back(instruction);
    }

    auto now = std::chrono::high_resolution_clock::now();
    console_log("Finished Parsing %i instruction(s) in [%ims]", instructions.size(), std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());

    /*  Control Flow Graph Section  */

    for (auto& i : instructions) {
        // TODO: do stuff here
    }
}
void MainWindow::on_textEdit_cursorPositionChanged()
{
    int x = ui->textEdit->textCursor().columnNumber() + 1;
    int y = ui->textEdit->textCursor().blockNumber() + 1;

    std::string message = "(Line: " + std::to_string(y) + ", Column: " + std::to_string(x) + ") ";

    ui->statusBar->showMessage(message.c_str());
}

// probably should do some worker thread stuff here, update: pain.
void MainWindow::on_textEdit_textChanged() { }
