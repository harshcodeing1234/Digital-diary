# Digital Diary - C++ Web Application

A full-stack digital diary application built with C++ backend, Oracle database, and modern web frontend.

## Features

- **User Authentication**: Secure login and registration system
- **Diary Management**: Create, read, update, and delete diary entries
- **Search Functionality**: Search through diary entries by keywords
- **Responsive UI**: Modern web interface with Tailwind CSS
- **Real-time Development**: Hot reload with nodemon for C++ files

## Tech Stack

### Backend
- **C++17**: Core application logic
- **Winsock2**: HTTP server implementation
- **Oracle Database**: Data persistence with OCCI (Oracle C++ Call Interface)

### Frontend
- **HTML5**: Structure and markup
- **Tailwind CSS**: Styling and responsive design
- **Vanilla JavaScript**: Client-side functionality
- **Font Awesome**: Icons

### Development Tools
- **CMake**: Build system
- **Nodemon**: Auto-restart on file changes
- **Visual Studio**: IDE support

## Project Structure

```
├── main.cpp              # HTTP server and request handling
├── db.cpp               # Database operations and Oracle connectivity
├── db.h                 # Database function declarations and structs
├── CMakeLists.txt       # CMake build configuration
├── .nodemon.json        # Nodemon configuration for hot reload
├── public/              # Static web assets
│   ├── index.html       # Main web interface
│   ├── script.js        # Client-side JavaScript
│   ├── style.css        # Custom styles
│   ├── Diary.png        # Application icon
│   └── diary_background.jpg # Background image
├── build/               # Compiled executables
└── .vs/                 # Visual Studio configuration
```

## Prerequisites

- **Oracle Instant Client 19.22** or later
- **Visual Studio** with C++ compiler (cl.exe)
- **CMake** 3.10 or later
- **Node.js** (for nodemon)

## Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd "landing page"
   ```

2. **Install Oracle Instant Client**
   - Download from Oracle website
   - Install to `C:\Program Files\Oracle\instantclient_19_22\`
   - Ensure SDK is included

3. **Install Node.js dependencies**
   ```bash
   npm install -g nodemon
   ```

4. **Configure Oracle paths**
   - Update paths in `CMakeLists.txt` if Oracle is installed elsewhere
   - Update paths in `.nodemon.json` compilation command

## Database Setup

The application uses Oracle database with the following schema:

### Tables
- **users**: User authentication data
- **diary_entries**: User diary entries with timestamps

### Database Functions (db.h)
- `createTables()`: Initialize database schema
- `registerUser()`: Create new user account
- `loginUser()`: Authenticate user credentials
- `insertEntry()`: Add new diary entry
- `fetchEntries()`: Retrieve user's entries
- `updateEntry()`: Modify existing entry
- `deleteEntry()`: Remove entry
- `searchEntries()`: Search entries by keyword

## Building and Running

### Using CMake
```bash
mkdir build
cd build
cmake ..
make
./blog_server
```

### Using Visual Studio Compiler (Recommended)
```bash
# Development mode with hot reload
nodemon

# Manual compilation
cl /EHsc /std:c++17 main.cpp db.cpp /I "C:\Program Files\Oracle\instantclient_19_22\sdk\include" /Fe:build\main.exe /link /LIBPATH:"C:\Program Files\Oracle\instantclient_19_22\sdk\lib\msvc" oraocci19.lib oci.lib

# nodemon compilation
nodemon --watch . --ext cpp,h,html --exec "cl /EHsc /std:c++17 main.cpp db.cpp /I \"C:\\Program Files\\Oracle\\instantclient_23_8\\sdk\\include\" /Fe:build\\main.exe /link /LIBPATH:\"C:\\Program Files\\Oracle\\instantclient_23_8\\sdk\\lib\\msvc\" oraocci23.lib oci.lib && build\\main.exe"

# Run
build\main.exe
```

## Usage 

1. **Start the server**
   ```bash
   nodemon  # or run the compiled executable
   ```

2. **Access the application**
   - Open browser to `http://localhost:8080`
   - Register a new account or login
   - Start creating diary entries

## API Endpoints

- `GET /` - Serve main application
- `POST /login` - User authentication
- `POST /register` - User registration
- `POST /entries` - Create new diary entry
- `GET /entries` - Fetch user's entries
- `PUT /entries` - Update existing entry
- `DELETE /entries` - Delete entry
- `GET /search` - Search entries

## Development

### Hot Reload
The project uses nodemon to automatically recompile and restart the server when C++ or HTML files change:

```json
{
  "watch": ["*.cpp", "*.h", "*.html"],
  "ext": "cpp,h,html",
  "exec": "compilation and run command"
}
```

### File Watching
- **C++ files**: `*.cpp`, `*.h`
- **Frontend files**: `*.html`
- Auto-compilation on save
- Automatic server restart

## Security Features

- Password hashing for user accounts
- Session management
- Input validation and sanitization
- SQL injection prevention through prepared statements

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

## License

This project is for educational purposes. Please ensure proper Oracle licensing for production use.

## Troubleshooting

### Common Issues

1. **Oracle connection errors**
   - Verify Oracle Instant Client installation
   - Check environment variables
   - Ensure database is running

2. **Compilation errors**
   - Verify Visual Studio installation
   - Check Oracle SDK paths in CMakeLists.txt
   - Ensure all dependencies are installed

3. **Port conflicts**
   - Default port is 8080
   - Modify in main.cpp if needed

### Build Artifacts

- `*.obj` files: Compiled object files
- `build/main.exe`: Final executable
- Generated during compilation process
