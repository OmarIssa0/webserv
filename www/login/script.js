const passwordInput = document.getElementById('password');
const eye           = document.getElementById('eye');
const loginForm     = document.getElementById('loginForm');

eye.addEventListener('click', () => {
    if (passwordInput.type === 'password') {
        passwordInput.type = 'text';
        eye.textContent    = 'Hide';
    } else {
        passwordInput.type = 'password';
        eye.textContent    = 'Show';
    }
});

loginForm.addEventListener('submit', async (event) => {
    event.preventDefault();

    const username = document.getElementById('username').value;
    const password = passwordInput.value;
        try {
            const response = await fetch(
                '/cgi-bin/database.py', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({username, password})});
            const result = await response.json();
            if (result.status === 'ok') {
                localStorage.setItem('username', username);
                window.location.href = '/welcome.html';
            } else {
                alert('Check your credentials and try again.');
            }
        } catch (error) {
            console.error(error);
            alert('An error occurred. Please try again later.');
        }
});
