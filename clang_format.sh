clang-format -style=WebKit -i `find ./common/ -name "*.h" | xargs`
clang-format -style=WebKit -i `find ./common/ -name "*.cpp" | xargs`
clang-format -style=WebKit -i `find ./example/ -name "*.h" | xargs`
clang-format -style=WebKit -i `find ./example/ -name "*.cpp" | xargs`
