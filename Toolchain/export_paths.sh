true_path="$(realpath "$(dirname "${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}")")"
export PATH="$true_path/CrossCompiler/Tools/bin:$PATH"