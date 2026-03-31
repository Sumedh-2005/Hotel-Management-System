const ROOM_IMGS = {
  s: ['https://images.unsplash.com/photo-1631049307264-da0ec9d70304?w=700&q=75',
      'https://images.unsplash.com/photo-1505693416388-ac5ce068fe85?w=700&q=75',
      'https://images.unsplash.com/photo-1522771739844-6a9f6d5f14af?w=700&q=75'],
  d: ['https://images.unsplash.com/photo-1571003123894-1f0594d2b5d9?w=700&q=75',
      'https://images.unsplash.com/photo-1578683010236-d716f9a3f461?w=700&q=75',
      'https://images.unsplash.com/photo-1560472354-b33ff0c44a43?w=700&q=75'],
  p: ['https://images.unsplash.com/photo-1551882547-ff40c63fe5fa?w=700&q=75',
      'https://images.unsplash.com/photo-1590490360182-c33d57733427?w=700&q=75',
      'https://images.unsplash.com/photo-1582719478250-c89cae4dc85b?w=700&q=75'],
};

const ROOMS_BASE = [
  { id:'SR101', number:'101', type:'Standard', name:'Classic Standard',        price:189, max:2, sqft:320,
    desc:'Elegant sanctuary with plush queen bed, marble bathroom, and panoramic city vistas.',
    long:'A carefully crafted sanctuary offering all the comforts of home. Features a plush queen bed with Egyptian cotton linens, marble-clad bathroom with rainfall shower, and floor-to-ceiling windows framing city vistas.',
    amenities:['Queen Bed','City View','Rain Shower','Mini Bar','4K Smart TV','Free WiFi'], img: ROOM_IMGS.s[0] },
  { id:'SR102', number:'102', type:'Standard', name:'Garden View Standard',    price:199, max:2, sqft:340,
    desc:'Serene garden-facing retreat with king bed and private terrace access.',
    long:'Serene and inviting with garden vistas, king bed dressed in fine linens, private terrace, and spa-inspired bathroom with soaking tub.',
    amenities:['King Bed','Garden View','Terrace','Soaking Tub','Smart TV','Free WiFi'], img: ROOM_IMGS.s[1] },
  { id:'SR103', number:'103', type:'Standard', name:'Twin Comfort Room',       price:179, max:2, sqft:300,
    desc:'Thoughtfully designed twin-bed room with dedicated workspace for traveling pairs.',
    long:'Two plush twin beds, generous workspace, curated minibar, and a well-appointed bathroom with premium Acqua di Parma toiletries.',
    amenities:['Twin Beds','Work Desk','City View','Mini Bar','Smart TV','Free WiFi'], img: ROOM_IMGS.s[2] },
  { id:'DR201', number:'201', type:'Deluxe',   name:'Deluxe Ocean View',       price:349, max:3, sqft:480,
    desc:'Floor-to-ceiling ocean panoramas with king bed, jacuzzi, and butler service.',
    long:'Elevated escape with full ocean panoramas. King bed, separate seating lounge, marble bathroom with deep soaking tub and walk-in rain shower. Deluxe amenities included.',
    amenities:['King Bed','Ocean View','Jacuzzi','Butler','Premium Bar','Rain Shower','4K TV'], img: ROOM_IMGS.d[0] },
  { id:'DR202', number:'202', type:'Deluxe',   name:'Deluxe Corner Suite',     price:389, max:3, sqft:520,
    desc:'Wraparound corner views with designer furnishings and a private living salon.',
    long:'Positioned at the building\'s coveted corner, panoramic views from two sides, king bed, sitting area with designer furniture, and spa-inspired bathroom.',
    amenities:['King Bed','Corner View','Living Area','Nespresso','Premium Bath','Smart TV'], img: ROOM_IMGS.d[1] },
  { id:'DR203', number:'203', type:'Deluxe',   name:'Garden Terrace Deluxe',   price:369, max:4, sqft:560,
    desc:'Private terrace with manicured garden access — a romantic ground-floor hideaway.',
    long:'Ground-level luxury with private terrace, outdoor dining, king bed, and premium bathroom with soaking tub surrounded by garden landscaping.',
    amenities:['King Bed','Private Terrace','Garden Access','Outdoor Dining','Soaking Tub'], img: ROOM_IMGS.d[2] },
  { id:'SS301', number:'301', type:'Penthouse', name:'Grand Horizon Suite',     price:1290, max:4, sqft:1800,
    desc:'Flagship two-bedroom suite with private dining room, home theatre, and panoramic skyline.',
    long:'The pinnacle of Grand Horizon luxury. Two-bedroom suite with private dining room for eight, full butler kitchen, home theatre system, and two master bathrooms. Complimentary airport transfer.',
    amenities:['2 Bedrooms','Panoramic View','Butler','Home Theatre','Chef\'s Kitchen','Airport Transfer','Jacuzzi'], img: ROOM_IMGS.p[0] },
  { id:'SS302', number:'302', type:'Penthouse', name:'Presidential Suite',      price:1890, max:4, sqft:2400,
    desc:'Heads-of-state grandeur with private pool, original art collection, and grand piano.',
    long:'Designed for the world\'s most discerning. Original art collection, private library, dedicated study, direct private pool access, two bedrooms, three bathrooms, and a Rolls-Royce transfer.',
    amenities:['2 Bedrooms','Private Pool','Art Collection','Grand Piano','Library','Butler 24/7','Rolls-Royce'], img: ROOM_IMGS.p[1] },
  { id:'SS303', number:'303', type:'Penthouse', name:'Horizon Sky Suite',       price:1490, max:4, sqft:2100,
    desc:'Rooftop sanctuary with private plunge pool, fire pit, and 360° city skyline views.',
    long:'Top-floor retreat with rooftop terrace, private plunge pool, fire pit, and 360° panorama. One master bedroom, chef\'s kitchen, sky lounge, and dedicated butler service.',
    amenities:['Master Bedroom','Rooftop Terrace','Plunge Pool','Fire Pit','Chef\'s Kitchen','Sky Lounge','Butler'], img: ROOM_IMGS.p[2] },
];

function getRooms() {
  if (typeof Storage === 'undefined') return ROOMS_BASE.map(r => ({ ...r, available: true }));
  const state = Storage.getRoomsState();
  const bookings = Storage.getBookings();
  return ROOMS_BASE.map(r => {
    const active = bookings.some(b =>
      b.roomId === r.id && b.status !== 'Cancelled' && new Date(b.checkOut) >= new Date()
    );
    return { ...r, available: !(state[r.id] === false || active) };
  });
}

const ROOMS = getRooms();

// ── Render cards ─────────────────────────────────
function renderRoomCards(container, rooms, showModal = false) {
  if (!container) return;
  if (!rooms.length) {
    container.innerHTML = `<div class="empty-state" style="grid-column:1/-1">
      <div class="empty-state-icon">◫</div>
      <div class="empty-state-text">No rooms found</div>
      <div class="empty-state-sub">Try changing your filters</div>
    </div>`;
    return;
  }
  container.innerHTML = rooms.map((r, i) => `
    <div class="room-card reveal reveal-delay-${Math.min(i+1,6)}" data-room-id="${r.id}">
      <div class="room-card-img">
        <img src="${r.img}" alt="${r.name}" loading="lazy">
        <div class="room-tag">${r.type}</div>
        <div class="room-avail ${r.available ? 'avail-yes' : 'avail-no'}">
          <span class="room-avail-dot"></span>${r.available ? 'Available' : 'Booked'}
        </div>
      </div>
      <div class="room-card-body">
        <div class="room-number">Rm ${r.number} &nbsp;·&nbsp; ${r.sqft} ft² &nbsp;·&nbsp; Up to ${r.max} guests</div>
        <div class="room-name">${r.name}</div>
        <p class="room-desc">${r.desc}</p>
        <div class="room-amenities">
          ${r.amenities.slice(0,4).map(a => `<span class="room-amenity">${a}</span>`).join('')}
          ${r.amenities.length > 4 ? `<span class="room-amenity text-amber">+${r.amenities.length-4}</span>` : ''}
        </div>
      </div>
      <div class="room-card-footer">
        <div class="room-price">
          <sup>₹</sup>${r.price.toLocaleString('en-IN')}<sub>&nbsp;/night</sub>
        </div>
        <div style="display:flex;gap:6px;align-items:center;">
          ${showModal ? `<button class="btn-book-sm" onclick="openRoomModal('${r.id}')">Details</button>` : ''}
          ${r.available
            ? `<a href="booking.html?room=${r.id}" class="btn-book-sm">Book</a>`
            : `<span class="btn-book-sm disabled">Booked</span>`}
        </div>
      </div>
    </div>
  `).join('');
  // Re-trigger reveal for newly inserted elements
  setTimeout(() => {
    const io = new IntersectionObserver(entries => {
      entries.forEach(en => { if (en.isIntersecting) { en.target.classList.add('visible'); io.unobserve(en.target); }});
    }, { threshold: 0.08 });
    container.querySelectorAll('.reveal').forEach(el => io.observe(el));
  }, 50);
}
