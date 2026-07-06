#ifndef SDF_PARSING_TEXT_DECODER_HPP
#define SDF_PARSING_TEXT_DECODER_HPP

#include <cstdint>
#include <span>
#include <string>

namespace sdf::parsing
{

std::string DecodeText(std::span<const std::uint8_t> chunk, bool compressed);

}

#endif
