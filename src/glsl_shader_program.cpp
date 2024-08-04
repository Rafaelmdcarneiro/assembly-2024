#include "glsl_shader_program.hpp"

#include "glsl_program.hpp"

#include <boost/throw_exception.hpp>

#include <iostream>
#include <sstream>

#if !defined(DNLOAD_GLESV2)

//######################################
// Global ##############################
//######################################

GlslShaderProgram::~GlslShaderProgram()
{
    cleanup();
}

void GlslShaderProgram::cleanup()
{
    if(m_id)
    {
        glDeleteProgram(m_id);
        m_id = 0u;
    }
}

bool GlslShaderProgram::compile()
{
    cleanup();

    std::string source = read();
    const GLchar* glsl_parts[1] = { source.c_str() };

    m_id = glCreateShaderProgramv(m_type, static_cast<GLsizei>(1), &(glsl_parts[0]));
    if(!get_program_link_status(m_id))
    {
        std::cout << get_string_with_prepended_line_numbers(source) << "\n----\n" << get_program_info_log(m_id);
        return false;
    }
    std::cout << "Program compiled: \"" << source << "\": " << m_id << std::endl;

    return true;
}

GLuint GlslShaderProgram::getStage() const
{
    switch(m_type)
    {
    case GL_VERTEX_SHADER:
        return GL_VERTEX_SHADER_BIT;

    case GL_FRAGMENT_SHADER:
        return GL_FRAGMENT_SHADER_BIT;

    case GL_MESH_SHADER_NV:
        return GL_MESH_SHADER_BIT_NV;

    default:
        break;
    }

    std::ostringstream sstr;
    sstr << "no stage known for shader of type '" << m_type << "'";
    BOOST_THROW_EXCEPTION(std::runtime_error(sstr.str()));
}

#endif

