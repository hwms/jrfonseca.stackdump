cmake -G "Visual Studio 9 2008" -H%CD% -B%CD%\build
"%SystemRoot%\Microsoft.NET\Framework\v3.5\MSBuild.exe" %CD%\build\STACKDUMP.sln /p:Configuration="RelWithDebInfo" /t:stackdump %*
