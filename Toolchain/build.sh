MYDIR="$(dirname "$(realpath "$0")")"

if [ -d "$MYDIR/CrossCompiler/some_file_that_indicates_that_its_built" ]
then
  echo "Cross compiler is already built!"
  exit 0
else
  echo "Building the cross compiler..."
fi

sudo apt update

declare -a dependencies=(
            "bison"
            "flex"
            "libgmp-dev"
            "libmpc-dev"
            "libmpfr-dev"
            "texinfo"
            "libisl-dev"
            "build-essential"
        )

for dependency in "${dependencies[@]}"
do
   echo -n $dependency
   is_dependency_installed=$(dpkg-query -l | grep $dependency)
   
   if [ -z "$is_dependency_installed" ]
    then
      echo " - not installed"
      sudo apt install -y $dependency
    else
      echo " - installed"
    fi
done

gcc_version="gcc-9.3.0"
binutils_version="binutils-2.34"
gcc_url="ftp://ftp.gnu.org/gnu/gcc/$gcc_version/$gcc_version.tar.gz"
binutils_url="https://ftp.gnu.org/gnu/binutils/$binutils_version.tar.gz"

if [ ! -e "$MYDIR/CrossCompiler/gcc/configure" ]
then
  echo "Downloading GCC source files..."
  mkdir -p "$MYDIR/CrossCompiler"
  mkdir -p "$MYDIR/CrossCompiler/gcc"
  wget -O "$MYDIR/CrossCompiler/gcc.tar.gz" $gcc_url
  echo "Unpacking GCC source files..."
  tar -xf "$MYDIR/CrossCompiler/gcc.tar.gz" -C  "$MYDIR/CrossCompiler/gcc" --strip-components 1
  rm "$MYDIR/CrossCompiler/gcc.tar.gz"
else
  echo "GCC is already downloaded!"
fi

if [ ! -e "$MYDIR/CrossCompiler/binutils/configure" ]
then
  echo "Downloading binutils source files..."
  mkdir -p  "$MYDIR/CrossCompiler"
  mkdir -p  "$MYDIR/CrossCompiler/binutils"
  wget -O "$MYDIR/CrossCompiler/binutils.tar.gz" $binutils_url
  echo "Unpacking binutils source files..."
  tar -xf "$MYDIR/CrossCompiler/binutils.tar.gz" -C "$MYDIR/CrossCompiler/binutils" --strip-components 1
  rm "$MYDIR/CrossCompiler/binutils.tar.gz"
else
  echo "binutils is already downloaded!"
fi

echo "Done!"