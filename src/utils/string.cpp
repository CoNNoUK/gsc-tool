#include "stdinc.hpp"

bool is_hex_number(const std::string& s)
{
	return !s.empty() && std::all_of(s.begin(), s.end(), ::isxdigit);
}

std::string to_lower(std::string input)
{
	std::string output(input.begin(), input.end());

	for (std::size_t i = 0; i < output.size(); i++)
	{
		output[i] = tolower(input[i]);
	}

	return output;
}

std::vector<std::string> split(std::string& str, char delimiter)
{
	std::vector<std::string> internal;
	std::stringstream ss(str);
	std::string tok;

	while (std::getline(ss, tok, delimiter))
	{
		internal.push_back(tok);
	}

	return internal;
}

std::vector<std::string> clean_buffer_lines(std::string& buffer)
{
	std::size_t pos;

	while ((pos = buffer.find("\t")) != std::string::npos)
	{
		buffer = buffer.replace(pos, 1, "");
	}
	while ((pos = buffer.find("\r")) != std::string::npos)
	{
		buffer = buffer.replace(pos, 1, "");
	}

	return split(buffer, '\n');
}

std::string get_string_literal(std::string str)
{
	return str.substr(1, str.size() - 2);
}