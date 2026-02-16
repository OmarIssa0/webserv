const passwordInput = document.getElementById('password');
const eye           = document.getElementById('eye');

eye.addEventListener('click', () => {
    if (passwordInput.type === 'password') {
        passwordInput.type = 'text';
        eye.textContent    = 'Hide';
    } else {
        passwordInput.type = 'password';
        eye.textContent    = 'Show';
    }
});
