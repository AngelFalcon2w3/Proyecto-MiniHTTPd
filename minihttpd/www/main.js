// Lógica JavaScript interactiva
document.addEventListener('DOMContentLoaded', () => {
    // 1. Buscador de Médicos en tiempo real
    const searchInput = document.getElementById('doctor-search');
    const doctorCards = document.querySelectorAll('.doctor-card');

    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            const query = e.target.value.toLowerCase();

            doctorCards.forEach(card => {
                const text = card.textContent.toLowerCase();
                if (text.includes(query)) {
                    card.style.display = 'block';
                } else {
                    card.style.display = 'none';
                }
            });
        });
    }

    // 2. Formulario de Reserva de Citas
    const appointmentForm = document.getElementById('appointment-form');
    const modal = document.getElementById('confirmation-modal');
    const modalDetails = document.getElementById('modal-details');
    const closeModalBtn = document.getElementById('close-modal');

    if (appointmentForm) {
        appointmentForm.addEventListener('submit', (e) => {
            e.preventDefault();

            const name = document.getElementById('name').value;
            const phone = document.getElementById('phone').value;
            const service = document.getElementById('service').value;
            const date = document.getElementById('date').value;

            modalDetails.innerHTML = `
                <strong>Paciente:</strong> ${name}<br>
                <strong>Especialidad:</strong> ${service}<br>
                <strong>Fecha:</strong> ${date}<br>
                <strong>Contacto:</strong> ${phone}<br><br>
                Nos comunicaremos en breve para confirmar la hora exacta.
            `;

            modal.classList.remove('hidden');
            appointmentForm.reset();
        });
    }

    if (closeModalBtn) {
        closeModalBtn.addEventListener('click', () => {
            modal.classList.add('hidden');
        });
    }
});
