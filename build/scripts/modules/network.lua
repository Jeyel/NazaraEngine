MODULE.Name = "Network"

MODULE.Libraries = {
	"NazaraCore"
}

MODULE.OsFiles.Windows = {
	"../src/Nazara/Network/Win32/**.hpp",
	"../src/Nazara/Network/Win32/**.cpp"
}

MODULE.OsFiles.Posix = {
	"../src/Nazara/Network/Posix/**.hpp",
	"../src/Nazara/Network/Posix/**.cpp"
}

MODULE.OsLibraries.Windows = {
	"ws2_32"
}
