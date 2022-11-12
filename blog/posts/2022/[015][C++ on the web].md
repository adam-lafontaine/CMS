# C++ on the web
## Running C/C++ application in a browser


### Installation

https://emscripten.org/docs/getting_started/downloads.html

Install SDL2

```plaintext
sudo apt-get install libsdl2-dev -y
```

Install Python

```plaintext
sudo apt-get install python3
```

Install Git

```plaintext
sudo apt-get install git
```

Clone the Emscripten SDK repo

```plaintext
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

```

Run the installation commands

```plaintext
# Enter that directory
cd emsdk

# Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
./emsdk install latest

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

From the terminal you will be working in

```plaintext
source ../../emsdk/emsdk_env.sh
```

### Makefile setup
