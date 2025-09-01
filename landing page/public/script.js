// Variables
let currentEntryId = null;
let entries = [];
let sessionToken = null;

// DOM Elements
const authScreen = document.getElementById('auth-screen');
const mainApp = document.getElementById('main-app');
const loginForm = document.getElementById('login-form');
const entryEditor = document.getElementById('entry-editor');
const welcomeScreen = document.getElementById('welcome-screen');
const entriesList = document.getElementById('entries-list');
const searchInput = document.getElementById('search-input');

// Session management
function setSessionToken(token) {
    sessionToken = token;
    localStorage.setItem('sessionToken', token);
}

function getSessionToken() {
    if (!sessionToken) {
        sessionToken = localStorage.getItem('sessionToken');
    }
    return sessionToken;
}

function clearSession() {
    sessionToken = null;
    localStorage.removeItem('sessionToken');
}

function handleUnauthorized() {
    alert('Your session has expired. Please log in again.');
    clearSession();
    showAuthScreen();
}

// Show/hide screens
function showAuthScreen() {
    authScreen.classList.remove('hidden');
    mainApp.classList.add('hidden');
}

function showMainApp() {
    authScreen.classList.add('hidden');
    mainApp.classList.remove('hidden');
}

// Initialization
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    setTodayDate();
    checkAutoLogin();
});

// Event listeners
function setupEventListeners() {
    // Auth
    loginForm.addEventListener('submit', handleLogin);
    document.getElementById('register-btn')?.addEventListener('click', handleRegister);
    document.getElementById('logout-btn').addEventListener('click', handleLogout);
    
    // Entry management
    document.getElementById('new-entry-btn').addEventListener('click', createNewEntry);
    document.getElementById('welcome-new-entry-btn').addEventListener('click', createNewEntry);
    document.getElementById('save-btn').addEventListener('click', saveEntry);
    document.getElementById('delete-btn').addEventListener('click', deleteEntry);
    
    // Search with debounce
    let searchTimeout;
    searchInput.addEventListener('input', () => {
        clearTimeout(searchTimeout);
        searchTimeout = setTimeout(handleSearch, 300);
    });
    
    // Export
    document.getElementById('export-btn').addEventListener('click', () => {
        document.getElementById('export-modal').classList.remove('hidden');
    });
    document.getElementById('cancel-export-btn').addEventListener('click', () => {
        document.getElementById('export-modal').classList.add('hidden');
    });
    document.getElementById('confirm-export-btn').addEventListener('click', handleExport);
    
    // User menu - fixed dropdown
    document.getElementById('user-menu-btn').addEventListener('click', (e) => {
        e.stopPropagation();
        document.getElementById('user-menu').classList.toggle('hidden');
    });
    
    // Close dropdown when clicking outside
    document.addEventListener('click', () => {
        document.getElementById('user-menu').classList.add('hidden');
    });
}

// Login
async function handleLogin(e) {
    e.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch('/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
        });

        const result = await response.text();

        if (result === 'LOGIN_SUCCESS') {
            // Extract session token from response headers
            const token = response.headers.get('Session-Token');
            if (token) {
                setSessionToken(token);
            }
            
            localStorage.setItem('loggedInUser', username);
            showMainApp(username);
        } else {
            alert('Login failed. Please check your credentials.');
        }
    } catch (error) {
        console.error('Login error:', error);
        alert('Login failed: Network error');
    }
}

// Register
async function handleRegister() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    
    if (!username || !password) {
        alert('Please enter username and password');
        return;
    }

    const response = await fetch('/register', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    });

    const result = await response.text();
    
    if (result === 'REGISTER_SUCCESS') {
        alert('Registration successful! You can now login.');
    } else {
        alert('Registration failed. Username might already exist.');
    }
}

function showMainApp(username) {
    document.getElementById('current-user').textContent = username;
    authScreen.classList.add('hidden');
    mainApp.classList.remove('hidden');
    loadEntries();
    updateGreeting();
}

// Logout
async function handleLogout() {
    await fetch('/logout');
    localStorage.removeItem('loggedInUser');
    mainApp.classList.add('hidden');
    authScreen.classList.remove('hidden');
    document.getElementById('login-form').reset();
    entries = [];
    currentEntryId = null;
}

function checkAutoLogin() {
    const user = localStorage.getItem('loggedInUser');
    if (user) {
        showMainApp(user);
    }
}

// Entries management
async function loadEntries() {
    try {
        const token = getSessionToken();
        if (!token) {
            handleUnauthorized();
            return;
        }

        const response = await fetch('/entry/view', {
            headers: {
                'Session-Token': token
            }
        });
        console.log('Load entries response:', response.status);
        
        if (!response.ok) {
            const errorText = await response.text();
            console.error('Load entries failed:', errorText);
            if (response.status === 401) {
                handleUnauthorized();
                return;
            } else if (response.status === 404) {
                // Session might be lost, redirect to login
                handleLogout();
                return;
            }
        }
        
        entries = await response.json();
        renderEntries(entries);
        updateStats();
    } catch (error) {
        console.error('Failed to load entries:', error);
        // Don't show alert for load failures, just log
    }
}

function formatDate(dateStr) {
    const options = { year: 'numeric', month: 'short', day: 'numeric' };
    return new Date(dateStr).toLocaleDateString(undefined, options);
}

function truncateText(text, maxLength = 50) {
    return text.length > maxLength ? text.substring(0, maxLength) + '...' : text;
}

// Render entries
function renderEntries(entriesToRender) {
    const fragment = document.createDocumentFragment();
    
    if (!entriesToRender.length) {
        entriesList.innerHTML = '<p class="text-gray-500 text-center py-4">No entries found</p>';
        return;
    }
    
    entriesToRender.forEach(entry => {
        const entryCard = document.createElement('div');
        entryCard.className = 'entry-card p-4 rounded-xl cursor-pointer border shadow-sm hover:shadow-md transition';

        const title = decodeURIComponent(entry.title) || 'Untitled';
        const content = decodeURIComponent(entry.content);
        
        entryCard.innerHTML = `
            <div class="flex justify-between items-center mb-1">
                <h4 class="font-semibold text-gray-800 truncate">${truncateText(title, 20)}</h4>
                <span class="text-sm text-gray-500">${formatDate(entry.entry_date)}</span>
            </div>
            <p class="text-xs text-gray-500 mt-1 truncate">${truncateText(content, 60)}</p>
        `;
        
        entryCard.addEventListener('click', () => editEntry(entry));
        fragment.appendChild(entryCard);
    });
    
    entriesList.innerHTML = '';
    entriesList.appendChild(fragment);
}

// Create entry
function createNewEntry() {
    currentEntryId = null;
    document.getElementById('editor-title').textContent = 'New Entry';
    document.getElementById('entry-title').value = '';
    document.getElementById('entry-content').value = '';
    document.getElementById('delete-btn').style.display = 'none';
    setTodayDate();
    showEditor();
}

// Edit entry
function editEntry(entry) {
    currentEntryId = entry.id;
    document.getElementById('editor-title').textContent = 'Edit Entry';
    document.getElementById('entry-title').value = decodeURIComponent(entry.title);
    document.getElementById('entry-content').value = decodeURIComponent(entry.content);
    document.getElementById('entry-date').value = entry.entry_date;
    document.getElementById('delete-btn').style.display = 'inline-block';
    showEditor();
}

// Show editor
function showEditor() {
    welcomeScreen.classList.add('hidden');
    entryEditor.classList.remove('hidden');
}

// Save entries
async function saveEntry() {
    const saveBtn = document.getElementById('save-btn');
    const originalText = saveBtn.textContent;
    
    try {
        saveBtn.textContent = 'Saving...';
        saveBtn.disabled = true;
        
        const title = document.getElementById('entry-title').value;
        const content = document.getElementById('entry-content').value;
        const entryDate = document.getElementById('entry-date').value;
        
        if (!title.trim() || !content.trim()) {
            alert('Please fill in both title and content');
            return;
        }
        
        const url = currentEntryId ? '/entry/edit' : '/entry/create';
        const body = currentEntryId 
            ? `id=${currentEntryId}&title=${encodeURIComponent(title)}&content=${encodeURIComponent(content)}&entry_date=${entryDate}`
            : `title=${encodeURIComponent(title)}&content=${encodeURIComponent(content)}&entry_date=${entryDate}`;
        
        const response = await fetch(url, {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/x-www-form-urlencoded',
                'Session-Token': getSessionToken() || ''
            },
            body: body
        });
        
        const responseText = await response.text();
        console.log('Save response:', response.status, responseText);
        
        if (response.ok) {
            document.getElementById('last-saved').textContent = `Last saved: ${new Date().toLocaleTimeString()}`;
            await loadEntries();
            alert('✅ Entry saved successfully!');
        } else if (response.status === 401 || responseText.includes('Unauthorized')) {
            alert('❌ Save failed: Your session has expired. Please log in again.');
            handleUnauthorized();
        } else {
            alert(`❌ Save failed: ${responseText}`);
        }
    } catch (error) {
        console.error('Save failed:', error);
        alert('Failed to save entry: Network error');
    } finally {
        saveBtn.textContent = originalText;
        saveBtn.disabled = false;
    }
}

// Delete entries
async function deleteEntry() {
    if (!currentEntryId || !confirm('Are you sure you want to delete this entry?')) return;
    
    try {
        const token = getSessionToken();
        if (!token) {
            handleUnauthorized();
            return;
        }

        const response = await fetch(`/entry/delete?id=${currentEntryId}`, {
            headers: {
                'Session-Token': token
            }
        });
        
        if (response.ok) {
            entryEditor.classList.add('hidden');
            welcomeScreen.classList.remove('hidden');
            await loadEntries();
            alert('✅ Entry deleted successfully!');
        } else if (response.status === 401) {
            handleUnauthorized();
        } else {
            const errorText = await response.text();
            alert(`❌ Delete failed: ${errorText}`);
        }
    } catch (error) {
        console.error('Delete failed:', error);
        alert('❌ Failed to delete entry: Network error');
    }
}

// Search entries
async function handleSearch() {
    const query = searchInput.value.trim();
    
    if (!query) {
        renderEntries(entries);
        return;
    }
    
    try {
        const token = getSessionToken();
        if (!token) {
            handleUnauthorized();
            return;
        }

        const response = await fetch(`/entry/search?q=${encodeURIComponent(query)}`, {
            headers: {
                'Session-Token': token
            }
        });
        
        if (response.status === 401) {
            handleUnauthorized();
            return;
        }
        
        if (response.ok) {
            const results = await response.json();
            renderEntries(results);
        } else {
            console.error('Search failed:', response.status);
        }
    } catch (error) {
        console.error('Search failed:', error);
    }
}

// Export entries
async function handleExport() {
    const format = document.querySelector('input[name="export-format"]:checked').value;
    
    try {
        const token = getSessionToken();
        if (!token) {
            handleUnauthorized();
            return;
        }

        const response = await fetch('/entry/export', {
            headers: {
                'Session-Token': token
            }
        });
        
        if (response.status === 401) {
            handleUnauthorized();
            return;
        }
        
        if (!response.ok) {
            const errorText = await response.text();
            alert(`❌ Export failed: ${errorText}`);
            return;
        }
        
        const data = await response.json();
        
        let content, filename, mimeType;
        
        if (format === 'json') {
            content = JSON.stringify(data, null, 2);
            filename = 'diary_entries.json';
            mimeType = 'application/json';
        } else if (format === 'txt') {
            content = data.map(entry => 
                `Date: ${formatDate(entry.entry_date)}\nTitle: ${decodeURIComponent(entry.title) || 'Untitled'}\n\nContent:\n${decodeURIComponent(entry.content)}\n\n${'-'.repeat(50)}\n\n`
            ).join('');
            filename = 'diary_entries.txt';
            mimeType = 'text/plain';
        } else {
            alert('PDF export not implemented yet');
            return;
        }
        
        const blob = new Blob([content], { type: mimeType });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
        
        document.getElementById('export-modal').classList.add('hidden');
        alert('✅ Entries exported successfully!');
    } catch (error) {
        console.error('Export failed:', error);
        alert('❌ Failed to export entries: Network error');
    }
}

// Statistics
function updateStats() {
    const totalEntries = entries.length;
    const totalWords = entries.reduce((sum, entry) => {
        const content = decodeURIComponent(entry.content);
        return sum + content.split(/\s+/).filter(word => word.length > 0).length;
    }, 0);
    const avgLength = totalEntries > 0 ? Math.round(totalWords / totalEntries) : 0;
    
    document.querySelector('.stat-box:nth-child(1) .stat-value').textContent = totalEntries;
    document.querySelector('.stat-box:nth-child(2) .stat-value').textContent = totalWords;
    document.querySelector('.stat-box:nth-child(3) .stat-value').textContent = avgLength || '-';
}

// Fill entry date input with today’s date.
function setTodayDate() {
    document.getElementById('entry-date').value = new Date().toISOString().split('T')[0];
}

// Show greeting with username.
function updateGreeting() {
    const user = localStorage.getItem('loggedInUser');
    document.getElementById('greeting').textContent = user ? `Welcome back, ${user}` : 'Welcome!';
}
