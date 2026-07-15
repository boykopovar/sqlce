#ifndef SDF_DOMAIN_TEXT_DECODER_HPP
#define SDF_DOMAIN_TEXT_DECODER_HPP

#include <cstdint>
#include <span>
#include <string>

namespace sdf::domain
{

std::string DecodeText(std::span<const std::uint8_t> chunk, bool compressed);

}

#endif
