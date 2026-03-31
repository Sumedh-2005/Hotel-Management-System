document.addEventListener('DOMContentLoaded', () => {
  let all = [], selectedId = null;

  function load() {
    all = Storage.getBookings();
    updateStats(); render(all);
    // Animate stat numbers
    document.querySelectorAll('[data-count]').forEach(el => {
      const target = parseFloat(el.dataset.count);
      if (!isNaN(target)) animateCount(el, target);
    });
  }

  function updateStats() {
    const conf = all.filter(b => b.status === 'Confirmed');
    const rev  = conf.reduce((s, b) => s + (b.total||0), 0);
    const avg  = conf.length ? conf.reduce((s,b) => s+(b.nights||0), 0) / conf.length : 0;
    const s = (id, v) => { const el = document.getElementById(id); if(el){ el.dataset.count = v; el.textContent = v; } };
    s('statTotal',     all.length);
    s('statConfirmed', conf.length);
    s('statRevenue',   rev.toFixed(0));
    s('statAvg',       avg.toFixed(1));
  }

  function render(bookings) {
    const tbody = document.getElementById('resTbody');
    if (!tbody) return;
    if (!bookings.length) {
      tbody.innerHTML = `<tr><td colspan="9">
        <div class="empty-state">
          <div class="empty-state-icon">◫</div>
          <div class="empty-state-text">No reservations yet</div>
          <div class="empty-state-sub">Make your first booking to see it here</div>
          <a href="booking.html" class="btn-primary" style="display:inline-flex;margin-top:12px;">Book a Room →</a>
        </div>
      </td></tr>`;
      return;
    }
    tbody.innerHTML = bookings.map(b => `
      <tr id="row-${b.id}" onclick="selectRow('${b.id}')">
        <td><strong class="font-mono" style="font-size:11px;letter-spacing:0.08em;">${b.id}</strong></td>
        <td><strong>${b.guestName||'—'}</strong></td>
        <td>
          <span style="display:block;font-size:9px;letter-spacing:0.15em;text-transform:uppercase;color:var(--amber);margin-bottom:2px;font-family:var(--ff-mono);">${b.roomType||''}</span>
          ${b.roomName||'—'} <span style="color:var(--text-dim);font-size:11px;">(${b.roomNumber||''})</span>
        </td>
        <td>${DateUtils.format(b.checkIn)}</td>
        <td>${DateUtils.format(b.checkOut)}</td>
        <td><span class="font-mono">${b.nights||0}</span></td>
        <td><strong>${fmt(b.total||0)}</strong></td>
        <td><span class="badge badge-${(b.status||'').toLowerCase()}">${b.status||'—'}</span></td>
        <td>
          <button class="action-btn" onclick="event.stopPropagation();selectRow('${b.id}')">View</button>
          ${b.status!=='Cancelled'
            ? `<button class="action-btn danger" onclick="event.stopPropagation();doCancelBooking('${b.id}')">Cancel</button>`
            : ''}
        </td>
      </tr>
    `).join('');
  }

  window.selectRow = id => {
    document.querySelectorAll('.res-table tbody tr').forEach(r => r.classList.remove('selected'));
    const row = document.getElementById('row-'+id);
    if (row) row.classList.add('selected');
    const b = all.find(x => x.id === id);
    if (!b) return;
    selectedId = id;

    const panel = document.getElementById('detailPanel');
    panel?.classList.add('open');
    panel?.scrollIntoView({ behavior:'smooth', block:'nearest' });

    setText('detailId',     `Ref: ${b.id}`);
    setText('detailName',   b.guestName||'—');
    setText('detailRoom',   b.roomName||'—');
    setText('detailIn',     DateUtils.format(b.checkIn));
    setText('detailOut',    DateUtils.format(b.checkOut));
    setText('detailNights', `${b.nights||0} night${b.nights!==1?'s':''}`);
    setText('detailTotal',  fmt(b.total||0));
    setText('detailStatus', b.status||'—');
    const statusEl = document.getElementById('detailStatus');
    if (statusEl) statusEl.className = `badge badge-${(b.status||'').toLowerCase()}`;

    setText('detailEmail',  b.email||'—');
    setText('detailPhone',  b.phone||'—');
    setText('detailPayment',b.paymentMethod||'—');
    setText('detailRate',   fmt(b.roomRate||0)+'/nt');
    setText('detailTax',    fmt(b.tax||0));
    setText('detailCreated',b.createdAt ? new Date(b.createdAt).toLocaleString() : '—');
    setText('detailSpecial',b.specialRequests||'None');

    const acts = document.getElementById('detailActions');
    if (acts) acts.innerHTML = `
      <button class="btn-ghost" style="font-size:10px;padding:10px 20px;" onclick="exportSingle('${b.id}')">Export JSON</button>
      ${b.status!=='Cancelled' ? `<button class="btn-primary" style="font-size:10px;padding:10px 20px;background:var(--danger);border:none;" onclick="doCancelBooking('${b.id}')">Cancel Booking</button>` : ''}
    `;
  };

  window.doCancelBooking = id => {
    if (!confirm(`Cancel booking ${id}? This cannot be undone.`)) return;
    const bookings = Storage.getBookings();
    const idx = bookings.findIndex(b => b.id === id);
    if (idx < 0) return;
    const b = bookings[idx];
    bookings[idx].status = 'Cancelled';
    Storage.saveBookings(bookings);
    Storage.setRoomAvail(b.roomId, true);
    localStorage.setItem('gh_last_output', JSON.stringify({type:'CANCEL_RESULT',bookingId:id,timestamp:new Date().toISOString()}));
    toast(`Booking ${id} cancelled.`, 'info');
    document.getElementById('detailPanel')?.classList.remove('open');
    load();
  };

  window.exportSingle = id => {
    const b = all.find(x => x.id === id);
    if (!b) return;
    exportJSON(b, `booking-${id}.json`);
    toast('Exported as JSON.', 'success');
  };

  // Search & filter
  function applySearch() {
    const q = (document.getElementById('searchInput')?.value||'').toLowerCase();
    const s = document.getElementById('statusFilter')?.value||'all';
    let res = all;
    if (s !== 'all') res = res.filter(b => b.status === s);
    if (q) res = res.filter(b =>
      (b.id||'').toLowerCase().includes(q) ||
      (b.guestName||'').toLowerCase().includes(q) ||
      (b.roomName||'').toLowerCase().includes(q) ||
      (b.email||'').toLowerCase().includes(q)
    );
    render(res);
  }
  document.getElementById('searchInput')?.addEventListener('input', applySearch);
  document.getElementById('statusFilter')?.addEventListener('change', applySearch);

  // Export all
  document.getElementById('exportAllBtn')?.addEventListener('click', () => {
    if (!all.length) { toast('No reservations to export.','info'); return; }
    exportJSON({ hotel:'Grand Horizon', exported: new Date().toISOString(), bookings: all }, `gh-reservations-${Date.now()}.json`);
    toast('All reservations exported!', 'success');
  });

  function setText(id, v) { const el = document.getElementById(id); if(el) el.textContent = v; }
  load();
});
