cmake -G "Visual Studio 9 2008" -H%CD% -B%CD%\build
"%SystemRoot%\Microsoft.NET\Framework\v3.5\MSBuild.exe" %CD%\build\SAMPLES.sln /p:Configuration="Debug" /t:ALL_BUILD %*
