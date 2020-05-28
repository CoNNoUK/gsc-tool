#pragma once

namespace gsc
{
	class assembler
	{
	public:
		assembler();

		void assemble(std::string& buffer);
		void assemble(std::vector<std::shared_ptr<function>>& functions);
		auto output_script() -> std::vector<std::uint8_t>;
		auto output_stack() -> std::vector<std::uint8_t>;

	private:
		std::unique_ptr<byte_buffer> m_script;
		std::unique_ptr<byte_buffer> m_stack;
		std::vector<std::shared_ptr<function>> m_functions;

		void assemble_function(std::shared_ptr<function> func);
		void assemble_instruction(std::shared_ptr<instruction> inst);
		void assemble_builtin_call(std::shared_ptr<instruction> inst, bool method, bool arg_num);
		void assemble_local_call(std::shared_ptr<instruction> inst, bool thread);
		void assemble_far_call(std::shared_ptr<instruction> inst, bool thread);
		void assemble_switch(std::shared_ptr<instruction> inst);
		void assemble_end_switch(std::shared_ptr<instruction> inst);
		void assemble_field_variable(std::shared_ptr<instruction> inst);
		void assemble_jump(std::shared_ptr<instruction> inst, bool expr, bool back);
		void assemble_offset(std::int32_t offset);
		auto resolve_function(const std::string& name) -> std::uint32_t;
		auto resolve_label(std::shared_ptr<instruction> inst, const std::string& name) -> std::uint32_t;
	};
}