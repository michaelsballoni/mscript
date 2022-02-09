# mscript
A simple, keyword-free scripting language for automating command line operations

It is useful for scripting that's too much for .bat files, and if Powershell or Python are unavailable or not necessary.

The thinking is, here's a simple scripting language, if it can solve your problem, then there's no need to get a bigger gun.

## objects

In mscript, every variable contains an object (think .NET's Object, but more like VB6's VARIANT)

An object can be one of six types of things:
1. null
2. number - double
3. string - std::wstring
4. bool
5. list - std::vector<object>
6. index - std::map<object, object>, with order of insertion preserved, a vectormap
