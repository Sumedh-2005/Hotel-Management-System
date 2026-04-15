# Grand Horizon — Hotel Management System

<<<<<<< Updated upstream
# Grand Horizon — Hotel Management System
=======
A full-stack Hotel Management System with a professional light-theme web frontend and a C++ OOP backend.

**Built by Sumedh Ajay Talokar**

---

## Folder Structure

```
hotel-management/
├── frontend/
│   ├── index.html          ← Home page
│   ├── rooms.html          ← Rooms listing with filter
│   ├── booking.html        ← Booking form
│   ├── reservations.html   ← My reservations dashboard
│   ├── css/style.css       ← Complete design system (light theme)
│   └── js/
│       ├── main.js         ← Shared utilities, storage, backend sim
│       ├── rooms.js        ← Room data + card rendering
│       ├── booking.js      ← Form validation + submission
│       └── reservations.js ← Table, search, cancel, export
│
├── backend/
│   ├── src/main.cpp        ← Complete C++ OOP backend
│   └── data/               ← JSON communication files (auto-created)
│
├── .gitignore
└── README.md
```

---

## Quick Start

### Frontend
```bash
# Open directly in any browser — no server needed
open frontend/index.html

# Or use a local server
cd frontend && python3 -m http.server 8080
# Visit: http://localhost:8080
```

### C++ Backend
```bash
cd backend

# Compile (requires g++ with C++17)
g++ -std=c++17 -o hotel src/main.cpp

# Run
./hotel
```

---


## Requirements

- **Frontend**: Any modern browser (Chrome, Firefox, Edge, Safari)
- **Backend**: `g++` with C++17
  - macOS: `xcode-select --install`
  - Ubuntu/Debian: `sudo apt install g++`
  - Windows: MinGW-w64

---

## Push to GitHub

```bash
git init
git add .
git commit -m "feat: Grand Horizon Hotel Management System"
git remote add origin https://github.com/YOUR_USERNAME/hotel-management.git
git branch -M main
git push -u origin main
```

---

*© 2024 Grand Horizon Hotel. Built by Sumedh Ajay Talokar*
>>>>>>> Stashed changes
