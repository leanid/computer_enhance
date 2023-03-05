#include <array>
#include <bitset>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

struct instruction
{
    explicit instruction(std::istream& binary)
        : bytes{}
    {
        using namespace std;
        if (!parse_from_binary_stream(binary))
        {
            stringstream ss;
            ss << "invalid binary for mov command ";
            string err = ss.str();
            cerr << err << endl;
        }
    }

    std::string_view name() const { return instruction_names[bytes.index()]; }

    std::string destanation_name() const
    {
        if (std::holds_alternative<register_memory_to_from_register>(bytes))
        {
            const auto& mov = std::get<register_memory_to_from_register>(bytes);
            if (mov.mod ==
                register_memory_to_from_register::mod_register_to_register)
            {
                unsigned reg = mov.d == 1 ? mov.reg : mov.r_m;
                return reg_names[mov.w][reg];
            }
            else if (mov.mod == register_memory_to_from_register::mod_memory &&
                     mov.r_m != 0b110)
            {
                return reg_names[mov.w][mov.reg];
            }
        }
        else if (std::holds_alternative<immediate_to_register>(bytes))
        {
            const auto& mov = std::get<immediate_to_register>(bytes);
            return reg_names[mov.w][mov.reg];
        }
        return "error";
    }

    std::string source_name() const
    {
        if (std::holds_alternative<register_memory_to_from_register>(bytes))
        {
            const auto& mov = std::get<register_memory_to_from_register>(bytes);
            if (mov.mod ==
                register_memory_to_from_register::mod_register_to_register)
            {

                unsigned reg = mov.d == 0 ? mov.reg : mov.r_m;
                return reg_names[mov.w][reg];
            }
            else if (mov.mod == register_memory_to_from_register::mod_memory &&
                     mov.r_m != 0b110)
            {
                return displacement_names[mov.r_m];
            }
        }
        else if (std::holds_alternative<immediate_to_register>(bytes))
        {
            const auto& mov = std::get<immediate_to_register>(bytes);
            uint16_t    value;
            if (mov.w == 1)
            {
                value = mov.data_if_w_1;
                value <<= 8;
            }
            value += mov.data;
            return std::to_string(value);
        }
        return "error";
    }

    operator bool() const { return bytes.index() != 0; }

private:
    [[nodiscard]] bool parse_from_binary_stream(std::istream& binary)
    {
        using namespace std;
        array<byte, 6> max_instruction_buffer{};

        if (!binary.read(reinterpret_cast<char*>(max_instruction_buffer.data()),
                         1))
        {
            if (binary.eof())
            {
                return true;
            }
            return false;
        }

        if (auto command = reinterpret_cast<register_memory_to_from_register*>(
                max_instruction_buffer.data());
            command->opcode == register_memory_to_from_register::prefix)
        {
            if (!binary.read(
                    reinterpret_cast<char*>(&max_instruction_buffer[1]), 1))
            {
                return false;
            }

            if (command->mod ==
                register_memory_to_from_register::mod_register_to_register)
            {
                bytes = *command;
                return true;
            }
            else if (command->mod ==
                         register_memory_to_from_register::mod_memory &&
                     command->r_m != 0b110)
            {
                bytes = *command;
                return true;
            }
        }
        else if (auto command = reinterpret_cast<immediate_to_register*>(
                     max_instruction_buffer.data());
                 command->opcode == immediate_to_register::prefix)
        {
            auto data_bytes_size = command->w == 1 ? 2 : 1;
            if (!binary.read(
                    reinterpret_cast<char*>(&max_instruction_buffer[1]),
                    data_bytes_size))
            {
                return false;
            }

            bytes = *command;
            return true;
        }

        return false;
    }
#pragma pack(push, 1)
    struct register_memory_to_from_register
    {
        static constexpr uint8_t prefix = 0b100010;
        // memory-mode no displacement (only if r_m==110 then 16-bit
        // displacement folows)
        static constexpr uint8_t mod_memory = 0b00;
        // memory-mode 8bit displacement folows
        static constexpr uint8_t mod_memory_8_bit = 0b01;
        // memory-mode 16bit displacement folows
        static constexpr uint8_t mod_memoty_16_bit = 0b10;
        // register-mode no displacement
        static constexpr uint8_t mod_register_to_register = 0b11;

        uint8_t w : 1;      /// word/byte operation 16 or 8 bit
        uint8_t d : 1;      /// Direction to or from register if 1 - reg is dst
        uint8_t opcode : 6; /// should be 100010

        uint8_t r_m : 3;
        uint8_t reg : 3;
        uint8_t mod : 2; /// should be 11

        uint8_t disp_lo : 8;

        uint8_t disp_hi : 8;
    };
    struct immediate_to_register
    {
        static constexpr uint8_t prefix = 0b1011;

        uint8_t reg : 3;
        uint8_t w : 1;
        uint8_t opcode : 4;

        uint8_t data : 8;

        uint8_t data_if_w_1 : 8;
    };
#pragma pack(pop)

    static_assert(sizeof(register_memory_to_from_register) == 4,
                  "should be exactly two bytes");

    static constexpr const char* reg_names[2][8] = {
        { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
        { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
    };
    static constexpr const char* displacement_names[8] = {
        "[bx + si]", "[bx + di]", "[bp + si]",     "[bp + di]",
        "[si]",      "[di]",      "direct access", "[bx]"
    };
    static constexpr const char* instruction_names[3] = { "invalid_state",
                                                          "mov",
                                                          "mov" };

    std::variant<std::monostate,
                 register_memory_to_from_register,
                 immediate_to_register>
        bytes;
};

std::ostream& operator<<(std::ostream& out, const instruction cmd)
{
    out << cmd.name() << " ";
    out << cmd.destanation_name() << ", " << cmd.source_name();
    return out;
}

int main(int argc, char** argv)
{
    using namespace std;
    using namespace std::literals;

    if (argc != 2)
    {
        cerr << "usage dec_mov <file>\n";
        return EXIT_FAILURE;
    }

    try
    {
        ifstream file;
        // file.exceptions(std::ios_base::failbit);
        file.open(argv[1], std::ios::binary);

        cout << "bits 16\n\n";

        for (instruction cmd(file); cmd; cmd = instruction(file))
        {
            cout << cmd << '\n';
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
