document.addEventListener('DOMContentLoaded', () => {
  const form        = document.getElementById('bookingForm');
  const roomSel     = document.getElementById('roomSelect');
  const checkIn     = document.getElementById('checkIn');
  const checkOut    = document.getElementById('checkOut');
  const payMethod   = document.getElementById('paymentMethod');
  const cardWrap    = document.getElementById('cardFields');
  const loadingEl   = document.getElementById('loadingOverlay');
  const confirmMod  = document.getElementById('confirmModal');

  // Populate room dropdown
  const rooms = getRooms();
  rooms.forEach(r => {
    const o = document.createElement('option');
    o.value = r.id;
    o.textContent = `${r.number} — ${r.name}  ($${r.price}/nt)${!r.available ? '  [Booked]' : ''}`;
    if (!r.available) o.disabled = true;
    roomSel.appendChild(o);
  });

  // URL param pre-select
  const pre = new URLSearchParams(location.search).get('room');
  if (pre && rooms.find(r => r.id === pre && r.available)) {
    roomSel.value = pre; updateSidebar(pre);
  }

  // Date constraints
  const today = DateUtils.today();
  checkIn.min = today; checkOut.min = today;

  // Card field toggle
  payMethod.addEventListener('change', () => {
    const show = ['Credit Card','Debit Card'].includes(payMethod.value);
    cardWrap.style.display = show ? 'grid' : 'none';
  });
  cardWrap.style.display = 'none';

  // Card number format
  document.getElementById('cardNumber')?.addEventListener('input', function() {
    this.value = this.value.replace(/\D/g,'').slice(0,16).replace(/(\d{4})(?=\d)/g,'$1 ');
  });

  // Sidebar update
  function updateSidebar(roomId) {
    const r = rooms.find(x => x.id === roomId);
    const preview = document.getElementById('roomPreview');
    const previewName = document.getElementById('previewName');
    const previewType = document.getElementById('previewType');
    const previewImg  = document.getElementById('previewImg');
    if (!r) { if(preview) preview.style.display='none'; return; }
    if (preview) preview.style.display = 'block';
    if (previewName) previewName.textContent = r.name;
    if (previewType) previewType.textContent = `${r.type} · Room ${r.number}`;
    if (previewImg)  { previewImg.src = r.img; previewImg.alt = r.name; }
    updatePrice(r);
  }

  function updatePrice(r) {
    if (!r) return;
    const ci = checkIn.value, co = checkOut.value;
    const n  = ci && co ? DateUtils.nightsBetween(ci, co) : 0;
    const sub = n * r.price, tax = +(sub * 0.12).toFixed(2), total = sub + tax;
    setText('priceRate',    `${fmt(r.price)}/nt`);
    setText('priceNights',  n);
    setText('priceSub',     fmt(sub));
    setText('priceTax',     fmt(tax));
    setText('priceTotal',   fmt(total));
  }
  function setText(id, v) { const el = document.getElementById(id); if(el) el.textContent = v; }

  roomSel.addEventListener('change',  () => updateSidebar(roomSel.value));
  checkIn.addEventListener('change',  () => {
    if (checkIn.value) {
      const d = new Date(checkIn.value); d.setDate(d.getDate()+1);
      checkOut.min = d.toISOString().split('T')[0];
      if (checkOut.value <= checkIn.value) checkOut.value = '';
    }
    const r = rooms.find(x => x.id === roomSel.value); if(r) updatePrice(r);
  });
  checkOut.addEventListener('change', () => {
    const r = rooms.find(x => x.id === roomSel.value); if(r) updatePrice(r);
  });

  // ── Validation ─────────────────────────────────
  function validate() {
    let ok = true;
    const checks = [
      ['firstName','firstNameErr', !V.required(val('firstName')), 'First name is required.'],
      ['lastName', 'lastNameErr',  !V.required(val('lastName')),  'Last name is required.'],
      ['email',    'emailErr',     !V.email(val('email')),        'Enter a valid email address.'],
      ['phone',    'phoneErr',     !V.phone(val('phone')),        'Enter a valid phone number.'],
      ['idNumber', 'idNumberErr',  !V.minLen(val('idNumber'),5),  'Enter a valid ID / passport number.'],
      ['guests',   'guestsErr',    !V.required(val('guests')),    'Select number of guests.'],
      ['roomSelect','roomErr',     !V.required(val('roomSelect')), 'Please select a room.'],
      ['checkIn',  'checkInErr',   !V.required(val('checkIn')),   'Select a check-in date.'],
      ['checkOut', 'checkOutErr',  !V.required(val('checkOut')) || val('checkOut') <= val('checkIn'), 'Check-out must be after check-in.'],
      ['paymentMethod','paymentErr', !V.required(val('paymentMethod')), 'Select a payment method.'],
    ];
    checks.forEach(([fid, eid, bad, msg]) => { if(!fieldError(fid, eid, bad, msg)) ok = false; });

    const pm = val('paymentMethod');
    if (['Credit Card','Debit Card'].includes(pm)) {
      if (!fieldError('cardNumber','cardNumberErr', !V.card(val('cardNumber')), 'Enter a valid 16-digit card number.')) ok = false;
      if (!fieldError('cardExpiry','cardExpiryErr', !V.expiry(val('cardExpiry')), 'Enter expiry date (MM/YY).')) ok = false;
    }

    const r = rooms.find(x => x.id === val('roomSelect'));
    if (r && +val('guests') > r.max) {
      fieldError('guests','guestsErr', true, `Max ${r.max} guests for this room.`); ok = false;
    }
    return ok;
  }
  const val = id => { const el = document.getElementById(id); return el ? el.value : ''; };

  // Blur-time inline validation
  ['firstName','lastName','idNumber'].forEach(id => {
    document.getElementById(id)?.addEventListener('blur', () =>
      fieldError(id, id+'Err', !V.required(val(id))));
  });
  document.getElementById('email')?.addEventListener('blur', () =>
    fieldError('email','emailErr', !V.email(val('email'))));
  document.getElementById('phone')?.addEventListener('blur', () =>
    fieldError('phone','phoneErr', !V.phone(val('phone'))));

  // ── Submit ─────────────────────────────────────
  form?.addEventListener('submit', async e => {
    e.preventDefault();
    if (!validate()) {
      toast('Please fix the highlighted errors.','error');
      form.querySelector('.has-error')?.scrollIntoView({ behavior:'smooth', block:'center' });
      return;
    }
    const r = rooms.find(x => x.id === val('roomSelect'));
    if (!r) return;

    const input = {
      firstName: val('firstName').trim(), lastName: val('lastName').trim(),
      email: val('email').trim(), phone: val('phone').trim(), idNumber: val('idNumber').trim(),
      guests: val('guests'), roomId: r.id, roomNumber: r.number, roomName: r.name,
      roomType: r.type, roomRate: r.price,
      checkIn: val('checkIn'), checkOut: val('checkOut'),
      paymentMethod: val('paymentMethod'),
      specialRequests: val('specialReq')?.trim() || '',
    };
    // Write "pending" file (simulated)
    localStorage.setItem('gh_pending', JSON.stringify(input));

    loadingEl?.classList.add('show');
    const result = await processBooking(input);
    loadingEl?.classList.remove('show');

    if (!result.success) { toast(result.error, 'error', 5000); return; }

    const b = result.booking;
    // Populate confirm modal
    setTextC('confirmId',    `Booking Ref: ${b.id}`);
    setTextC('confirmGuest', b.guestName);
    setTextC('confirmRoom',  b.roomName);
    setTextC('confirmIn',    DateUtils.format(b.checkIn));
    setTextC('confirmOut',   DateUtils.format(b.checkOut));
    setTextC('confirmNights',`${b.nights} night${b.nights!==1?'s':''}`);
    setTextC('confirmTotal', fmt(b.total));
    confirmMod?.classList.add('open');
    form.reset();
    cardWrap.style.display = 'none';
    document.getElementById('roomPreview').style.display = 'none';
    ['priceRate','priceNights','priceSub','priceTax','priceTotal'].forEach(id => setText(id,'—'));
  });

  function setTextC(id, v) { const el = document.getElementById(id); if(el) el.textContent = v; }

  // Close confirm modal
  document.getElementById('confirmClose')?.addEventListener('click', () => confirmMod?.classList.remove('open'));
  confirmMod?.addEventListener('click', e => { if(e.target === confirmMod) confirmMod.classList.remove('open'); });
});
