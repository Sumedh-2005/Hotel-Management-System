// ── Navbar scroll ──────────────────────────────
(function initNav() {
  const nav = document.querySelector('.nav');
  if (!nav) return;
  const toggle = () => nav.classList.toggle('scrolled', window.scrollY > 20);
  window.addEventListener('scroll', toggle, { passive: true });
  toggle();
})();

// ── Scroll reveal ─────────────────────────────
(function initReveal() {
  const io = new IntersectionObserver((entries) => {
    entries.forEach(en => {
      if (en.isIntersecting) { en.target.classList.add('visible'); io.unobserve(en.target); }
    });
  }, { threshold: 0.12 });
  document.querySelectorAll('.reveal').forEach(el => io.observe(el));
})();

// ── Hero parallax bg ──────────────────────────
(function initHeroBg() {
  const bg = document.querySelector('.hero-bg');
  if (!bg) return;
  bg.style.transform = 'scale(1.04)';
  window.addEventListener('scroll', () => {
    const progress = Math.min(window.scrollY / window.innerHeight, 1);
    bg.style.transform = `scale(${1.04 + progress * 0.04})`;
    bg.style.opacity = 1 - progress * 0.4;
  }, { passive: true });
})();

// ── Counter animation ─────────────────────────
function animateCount(el, target, duration = 1200) {
  const start = performance.now();
  const isFloat = target % 1 !== 0;
  const step = (now) => {
    const p = Math.min((now - start) / duration, 1);
    const ease = 1 - Math.pow(1 - p, 3);
    const val = target * ease;
    el.textContent = isFloat ? val.toFixed(1) : Math.round(val).toLocaleString();
    if (p < 1) requestAnimationFrame(step);
  };
  requestAnimationFrame(step);
}
(function initCounters() {
  const io = new IntersectionObserver((entries) => {
    entries.forEach(en => {
      if (!en.isIntersecting) return;
      const el = en.target;
      const target = parseFloat(el.dataset.count || el.textContent);
      if (!isNaN(target)) animateCount(el, target);
      io.unobserve(el);
    });
  }, { threshold: 0.5 });
  document.querySelectorAll('[data-count]').forEach(el => io.observe(el));
})();

// ── Storage ────────────────────────────────────
const Storage = {
  get(k)        { try { return JSON.parse(localStorage.getItem(k)); } catch { return null; } },
  set(k, v)     { try { localStorage.setItem(k, JSON.stringify(v)); return true; } catch { return false; } },
  getBookings() { return this.get('gh_bookings') || []; },
  saveBookings(b) { return this.set('gh_bookings', b); },
  getRoomsState() { return this.get('gh_rooms_state') || {}; },
  setRoomAvail(id, avail) {
    const s = this.getRoomsState(); s[id] = avail; this.set('gh_rooms_state', s);
  },
};

// ── Date utils ─────────────────────────────────
const DateUtils = {
  today()           { return new Date().toISOString().split('T')[0]; },
  nightsBetween(ci, co) { return Math.max(0, Math.round((new Date(co) - new Date(ci)) / 86400000)); },
  format(d)         {
    if (!d) return '—';
    return new Date(d + 'T00:00:00').toLocaleDateString('en-US', { day:'numeric', month:'short', year:'numeric' });
  },
  overlaps(s1, e1, s2, e2) { return s1 < e2 && e1 > s2; },
};

// ── Currency ────────────────────────────────────
const fmt = v => '₹' + Number(v).toLocaleString('en-IN', { maximumFractionDigits: 2, minimumFractionDigits: 2 });

// ── ID gen ─────────────────────────────────────
const genId = () => 'GH-' + Date.now().toString(36).toUpperCase() + '-' + Math.random().toString(36).substr(2,4).toUpperCase();

// ── Double booking check ────────────────────────
function isDoubleBooked(roomId, ci, co, excludeId = null) {
  return Storage.getBookings().some(b => {
    if (b.status === 'Cancelled') return false;
    if (excludeId && b.id === excludeId) return false;
    if (b.roomId !== roomId) return false;
    return DateUtils.overlaps(ci, co, b.checkIn, b.checkOut);
  });
}

// ── Validation helpers ─────────────────────────
const V = {
  required: v => !!(v && v.trim()),
  email:    v => /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(v),
  phone:    v => /^[\+]?[\d\s\-\(\)]{7,15}$/.test(v.trim()),
  card:     v => /^\d{4}\s?\d{4}\s?\d{4}\s?\d{4}$/.test(v.trim()),
  expiry:   v => /^\d{2}\/\d{2}/.test(v.trim()),
  minLen:   (v, n) => v && v.trim().length >= n,
};
function fieldError(fieldId, errId, show, msg = '') {
  const f = document.getElementById(fieldId);
  const e = document.getElementById(errId);
  f?.classList.toggle('has-error', show);
  if (e) { e.classList.toggle('show', show); if (msg) e.textContent = msg; }
  return !show;
}

// ── Toast ──────────────────────────────────────
function toast(msg, type = 'info', ms = 3800) {
  const icons = { success: '✓', error: '✗', info: '◆', warning: '⚠' };
  const c = document.getElementById('toastContainer');
  if (!c) return;
  const t = document.createElement('div');
  t.className = `toast ${type}`;
  t.innerHTML = `<span class="toast-icon">${icons[type]||icons.info}</span><span>${msg}</span>`;
  c.appendChild(t);
  setTimeout(() => {
    t.style.transition = 'opacity 0.3s, transform 0.3s';
    t.style.opacity = '0'; t.style.transform = 'translateX(12px)';
    setTimeout(() => t.remove(), 320);
  }, ms);
}

// ── Export JSON file ───────────────────────────
function exportJSON(data, name) {
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' }));
  a.download = name; a.click();
}

// ── Backend simulation (mirrors C++ logic) ─────
async function processBooking(input) {
  await new Promise(r => setTimeout(r, 1600)); // simulate C++ file read/process/write
  const nights = DateUtils.nightsBetween(input.checkIn, input.checkOut);
  if (nights < 1) return { success: false, error: 'Check-out must be after check-in.' };
  if (isDoubleBooked(input.roomId, input.checkIn, input.checkOut))
    return { success: false, error: `Room ${input.roomNumber} is already booked for those dates.` };
  const sub = nights * input.roomRate;
  const tax = +(sub * 0.12).toFixed(2);
  const total = +(sub + tax).toFixed(2);
  const b = {
    id: genId(),
    guestName: `${input.firstName} ${input.lastName}`,
    email: input.email, phone: input.phone, idNumber: input.idNumber,
    guests: +input.guests,
    roomId: input.roomId, roomNumber: input.roomNumber,
    roomName: input.roomName, roomType: input.roomType,
    checkIn: input.checkIn, checkOut: input.checkOut,
    nights, roomRate: input.roomRate,
    subtotal: sub, tax, total,
    paymentMethod: input.paymentMethod,
    specialRequests: input.specialRequests || '',
    status: 'Confirmed',
    createdAt: new Date().toISOString(),
  };
  Storage.saveBookings([...Storage.getBookings(), b]);
  Storage.setRoomAvail(input.roomId, false);
  localStorage.setItem('gh_last_output', JSON.stringify({ type:'BOOKING_RESULT', booking: b, timestamp: b.createdAt }));
  return { success: true, booking: b };
}
