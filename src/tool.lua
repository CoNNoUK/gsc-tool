tool = {}

function tool:include()
    includedirs {
        path.join(project_dir(), "tool")
    }
end

function tool:link()
    self:include()
	
    links
    {
		"gsc-tool"
	}
end

function tool:project()
    local folder = project_dir();

    project "gsc-tool"
        kind "ConsoleApp"
        language "C++"
        
        pchheader "stdinc.hpp"
        pchsource(path.join(folder, "tool/stdinc.cpp"))

        files {
            path.join(folder, "tool/**.h"),
            path.join(folder, "tool/**.hpp"),
            path.join(folder, "tool/**.cpp")
        }

        -- Linked projects
        self:include()
        utils:link()
        IW5:link()
        IW6:link()
        SH1:link()

end