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

struct cmd_mov
{
    explicit cmd_mov(std::array<std::byte, 2> binary)
        : bytes{ *reinterpret_cast<binary_representation*>(binary.data()) }
    {
        using namespace std;
        if (!is_mov_instruction())
        {
            stringstream ss;
            ss << "invalid binary for mov command [0x" << hex
               << static_cast<uint32_t>(binary[0]) << " 0x"
               << static_cast<uint32_t>(binary[1]) << "] or [0b"
               << bitset<8>(static_cast<uint8_t>(binary[0])) << " 0b"
               << bitset<8>(static_cast<uint8_t>(binary[1])) << "] "
               << "opcode: 0b" << bitset<6>(bytes.opcode) << " "
               << "d: 0b" << bitset<1>(bytes.d) << " "
               << "w: 0b" << bitset<1>(bytes.w) << " "
               << "mod: 0b" << bitset<2>(bytes.mod) << " "
               << "reg: 0b" << bitset<3>(bytes.reg) << " "
               << "r/m: 0b" << bitset<3>(bytes.r_m);
            string err = ss.str();
            throw runtime_error(err);
        }
    }
    bool is_mov_instruction() const { return bytes.opcode == mov_prefix; }
    static constexpr uint8_t mov_prefix = 0b100010;

    std::string_view destanation_name() const
    {
        unsigned reg = bytes.d == 1 ? bytes.reg : bytes.r_m;
        return reg_names[bytes.w][reg];
    }

    std::string_view source_name() const
    {
        unsigned reg = bytes.d == 0 ? bytes.reg : bytes.r_m;
        return reg_names[bytes.w][reg];
    }

#pragma pack(push, 1)
    struct binary_representation
    {
        uint8_t w : 1; /// word/byte operation 16 or 8 bit
        uint8_t d : 1; /// Direction to or from register if 1 - reg is dst
        uint8_t opcode : 6;

        uint8_t r_m : 3;
        uint8_t reg : 3;
        uint8_t mod : 2;
    };
#pragma pack(pop)

    static_assert(sizeof(binary_representation) == 2,
                  "should be exactly two bytes");

    static constexpr const char* reg_names[2][8] = {
        { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
        { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
    };

    binary_representation bytes;
};

std::ostream& operator<<(std::ostream& out, const cmd_mov cmd)
{
    out << "mov ";
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

        array<byte, 2> mov_command;
        while (file.read(reinterpret_cast<char*>(mov_command.data()),
                         mov_command.size()))
        {
            cmd_mov mov(mov_command);
            cout << mov << '\n';
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
