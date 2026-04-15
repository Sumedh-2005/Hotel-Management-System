/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║        GRAND HORIZON — C++ Hotel Management Backend         ║
 * ║  OOP-based simulation with file I/O and JSON processing     ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * Compile:  g++ -std=c++17 -o hotel main.cpp
 * Run:      ./hotel
 *
 * File Integration:
 *   Input:  data/pending_booking.json  (written by frontend)
 *   Output: data/booking_result.json   (read by frontend)
 *   Store:  data/bookings.json         (persistent storage)
 *   Rooms:  data/rooms.json            (room inventory)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <random>

// ═══════════════════════════════════════════════════════════════
// SECTION 1: JSON MINI-PARSER (no external dependencies)
// ═══════════════════════════════════════════════════════════════

class JSON {
public:
    enum Type { STRING, NUMBER, BOOL, OBJECT, ARRAY, NIL };

    Type type;
    std::string str_val;
    double num_val = 0;
    bool bool_val = false;
    std::map<std::string, JSON> obj;
    std::vector<JSON> arr;

    JSON() : type(NIL) {}
    explicit JSON(const std::string& s) : type(STRING), str_val(s) {}
    explicit JSON(double n) : type(NUMBER), num_val(n) {}
    explicit JSON(bool b) : type(BOOL), bool_val(b) {}

    std::string getString(const std::string& key, const std::string& def = "") const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == STRING)
                return it->second.str_val;
        }
        return def;
    }

    double getNumber(const std::string& key, double def = 0) const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == NUMBER)
                return it->second.num_val;
        }
        return def;
    }

    bool getBool(const std::string& key, bool def = false) const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == BOOL)
                return it->second.bool_val;
        }
        return def;
    }

    // Simple JSON serializer
    std::string serialize(int indent = 0) const {
        std::string pad(indent * 2, ' ');
        std::string childPad((indent + 1) * 2, ' ');

        switch (type) {
            case STRING: return "\"" + escapeStr(str_val) + "\"";
            case NUMBER: {
                std::ostringstream oss;
                if (num_val == (long long)num_val) oss << (long long)num_val;
                else oss << std::fixed << std::setprecision(2) << num_val;
                return oss.str();
            }
            case BOOL: return bool_val ? "true" : "false";
            case NIL: return "null";
            case OBJECT: {
                if (obj.empty()) return "{}";
                std::string out = "{\n";
                bool first = true;
                for (const auto& [k, v] : obj) {
                    if (!first) out += ",\n";
                    out += childPad + "\"" + k + "\": " + v.serialize(indent + 1);
                    first = false;
                }
                return out + "\n" + pad + "}";
            }
            case ARRAY: {
                if (arr.empty()) return "[]";
                std::string out = "[\n";
                bool first = true;
                for (const auto& v : arr) {
                    if (!first) out += ",\n";
                    out += childPad + v.serialize(indent + 1);
                    first = false;
                }
                return out + "\n" + pad + "]";
            }
        }
        return "null";
    }

private:
    static std::string escapeStr(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else out += c;
        }
        return out;
    }

public:
    // Simple parser (handles flat objects and arrays of objects)
    static JSON parse(const std::string& input) {
        size_t pos = 0;
        return parseValue(input, pos);
    }

private:
    static void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos]==' '||s[pos]=='\n'||s[pos]=='\r'||s[pos]=='\t')) pos++;
    }

    static std::string parseString(const std::string& s, size_t& pos) {
        pos++; // skip opening "
        std::string out;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\') { pos++; if (pos < s.size()) out += s[pos]; }
            else out += s[pos];
            pos++;
        }
        pos++; // skip closing "
        return out;
    }

    static JSON parseValue(const std::string& s, size_t& pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) return JSON();

        if (s[pos] == '{') return parseObject(s, pos);
        if (s[pos] == '[') return parseArray(s, pos);
        if (s[pos] == '"') {
            std::string str = parseString(s, pos);
            JSON j; j.type = STRING; j.str_val = str; return j;
        }
        if (s[pos] == 't') { pos += 4; JSON j; j.type = BOOL; j.bool_val = true; return j; }
        if (s[pos] == 'f') { pos += 5; JSON j; j.type = BOOL; j.bool_val = false; return j; }
        if (s[pos] == 'n') { pos += 4; return JSON(); }
        // number
        std::string numStr;
        while (pos < s.size() && (isdigit(s[pos])||s[pos]=='-'||s[pos]=='.'||s[pos]=='e'||s[pos]=='E'||s[pos]=='+'))
            numStr += s[pos++];
        JSON j; j.type = NUMBER; j.num_val = std::stod(numStr); return j;
    }

    static JSON parseObject(const std::string& s, size_t& pos) {
        pos++; // skip {
        JSON obj; obj.type = OBJECT;
        skipWhitespace(s, pos);
        while (pos < s.size() && s[pos] != '}') {
            skipWhitespace(s, pos);
            if (s[pos] != '"') { pos++; continue; }
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ':') pos++;
            skipWhitespace(s, pos);
            obj.obj[key] = parseValue(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            skipWhitespace(s, pos);
        }
        if (pos < s.size()) pos++; // skip }
        return obj;
    }

    static JSON parseArray(const std::string& s, size_t& pos) {
        pos++; // skip [
        JSON arr; arr.type = ARRAY;
        skipWhitespace(s, pos);
        while (pos < s.size() && s[pos] != ']') {
            arr.arr.push_back(parseValue(s, pos));
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            skipWhitespace(s, pos);
        }
        if (pos < s.size()) pos++; // skip ]
        return arr;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 2: BASE ROOM CLASS + DERIVED TYPES
// ═══════════════════════════════════════════════════════════════

class Room {
protected:
    std::string roomId;
    std::string roomNumber;
    std::string roomName;
    double basePrice;
    int maxGuests;
    int sqft;
    bool available;
    std::vector<std::string> amenities;

public:
    Room(const std::string& id, const std::string& num, const std::string& name,
         double price, int maxG, int sqFt)
        : roomId(id), roomNumber(num), roomName(name),
          basePrice(price), maxGuests(maxG), sqft(sqFt), available(true) {}

    virtual ~Room() = default;

    // Getters
    std::string getId() const { return roomId; }
    std::string getNumber() const { return roomNumber; }
    std::string getName() const { return roomName; }
    double getBasePrice() const { return basePrice; }
    int getMaxGuests() const { return maxGuests; }
    bool isAvailable() const { return available; }

    void setAvailable(bool avail) { available = avail; }
    void addAmenity(const std::string& a) { amenities.push_back(a); }

    // Virtual methods — overridden by derived classes
    virtual std::string getType() const = 0;
    virtual double calculateNightlyRate(int nights) const {
        // Base: flat rate
        return basePrice;
    }
    virtual double calculateTotal(int nights) const {
        return calculateNightlyRate(nights) * nights;
    }
    virtual std::string getDescription() const = 0;

    // Display info
    virtual void display() const {
        std::cout << "  [" << getType() << "] Room " << roomNumber
                  << " — " << roomName << "\n"
                  << "  Price: $" << std::fixed << std::setprecision(2)
                  << basePrice << "/night | Max guests: " << maxGuests
                  << " | " << sqft << " sq ft"
                  << " | " << (available ? "Available" : "Booked") << "\n";
    }

    JSON toJSON() const {
        JSON j; j.type = JSON::OBJECT;
        j.obj["id"] = JSON(roomId);
        j.obj["number"] = JSON(roomNumber);
        j.obj["name"] = JSON(roomName);
        j.obj["type"] = JSON(getType());
        j.obj["price"] = JSON(basePrice);
        j.obj["maxGuests"] = JSON((double)maxGuests);
        j.obj["sqft"] = JSON((double)sqft);
        j.obj["available"] = JSON(available);
        j.obj["description"] = JSON(getDescription());
        JSON amenArr; amenArr.type = JSON::ARRAY;
        for (const auto& a : amenities) amenArr.arr.push_back(JSON(a));
        j.obj["amenities"] = amenArr;
        return j;
    }
};

// ─── Standard Room ────────────────────────────────────────────
class StandardRoom : public Room {
public:
    StandardRoom(const std::string& id, const std::string& num, const std::string& name,
                 double price, int maxG = 2, int sqFt = 320)
        : Room(id, num, name, price, maxG, sqFt) {}

    std::string getType() const override { return "Standard"; }
    std::string getDescription() const override {
        return "Comfortable standard room with essential amenities and modern decor.";
    }
    double calculateNightlyRate(int nights) const override {
        // Standard: flat rate with 5% discount for 7+ nights
        return nights >= 7 ? basePrice * 0.95 : basePrice;
    }
};

// ─── Deluxe Room ─────────────────────────────────────────────
class DeluxeRoom : public Room {
    bool hasOceanView;
public:
    DeluxeRoom(const std::string& id, const std::string& num, const std::string& name,
               double price, bool oceanView = false, int maxG = 3, int sqFt = 480)
        : Room(id, num, name, price, maxG, sqFt), hasOceanView(oceanView) {}

    std::string getType() const override { return "Deluxe"; }
    std::string getDescription() const override {
        return "Premium deluxe room with superior furnishings and stunning views.";
    }
    double calculateNightlyRate(int nights) const override {
        // Deluxe: 8% discount for 5+ nights, 15% for 10+
        if (nights >= 10) return basePrice * 0.85;
        if (nights >= 5)  return basePrice * 0.92;
        return basePrice;
    }
    bool getOceanView() const { return hasOceanView; }
};

// ─── Suite Room ───────────────────────────────────────────────
class SuiteRoom : public Room {
    int numBedrooms;
    bool hasButler;
public:
    SuiteRoom(const std::string& id, const std::string& num, const std::string& name,
              double price, int bedrooms = 1, bool butler = false, int maxG = 4, int sqFt = 1800)
        : Room(id, num, name, price, maxG, sqFt), numBedrooms(bedrooms), hasButler(butler) {}

    std::string getType() const override { return "Suite"; }
    std::string getDescription() const override {
        return std::to_string(numBedrooms) + "-bedroom luxury suite with premium services.";
    }
    double calculateNightlyRate(int nights) const override {
        // Suite: 10% off for 3+ nights, butler surcharge applied separately
        double rate = nights >= 3 ? basePrice * 0.90 : basePrice;
        return rate;
    }
    int getBedrooms() const { return numBedrooms; }
    bool getHasButler() const { return hasButler; }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 3: GUEST CLASS
// ═══════════════════════════════════════════════════════════════

class Guest {
    std::string firstName, lastName, email, phone, idNumber;
    int numGuests;

public:
    Guest() : numGuests(1) {}
    Guest(const std::string& fn, const std::string& ln,
          const std::string& em, const std::string& ph,
          const std::string& id, int n)
        : firstName(fn), lastName(ln), email(em),
          phone(ph), idNumber(id), numGuests(n) {}

    // Getters
    std::string getFullName() const { return firstName + " " + lastName; }
    std::string getFirstName() const { return firstName; }
    std::string getLastName() const { return lastName; }
    std::string getEmail() const { return email; }
    std::string getPhone() const { return phone; }
    std::string getIdNumber() const { return idNumber; }
    int getNumGuests() const { return numGuests; }

    // Validation
    bool isValid() const {
        return !firstName.empty() && !lastName.empty() &&
               email.find('@') != std::string::npos &&
               phone.length() >= 7 && idNumber.length() >= 5;
    }

    void display() const {
        std::cout << "  Guest: " << getFullName()
                  << " | Email: " << email
                  << " | ID: " << idNumber
                  << " | Party of " << numGuests << "\n";
    }

    JSON toJSON() const {
        JSON j; j.type = JSON::OBJECT;
        j.obj["firstName"] = JSON(firstName);
        j.obj["lastName"] = JSON(lastName);
        j.obj["fullName"] = JSON(getFullName());
        j.obj["email"] = JSON(email);
        j.obj["phone"] = JSON(phone);
        j.obj["idNumber"] = JSON(idNumber);
        j.obj["numGuests"] = JSON((double)numGuests);
        return j;
    }

    static Guest fromJSON(const JSON& j) {
        return Guest(
            j.getString("firstName"),
            j.getString("lastName"),
            j.getString("email"),
            j.getString("phone"),
            j.getString("idNumber"),
            (int)j.getNumber("guests", 1)
        );
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 4: PAYMENT CLASS
// ═══════════════════════════════════════════════════════════════

class Payment {
public:
    enum Status { PENDING, COMPLETED, FAILED, REFUNDED };

private:
    std::string method;
    double amount;
    double tax;
    double total;
    Status status;
    std::string transactionId;

    static const double TAX_RATE;

public:
    Payment() : amount(0), tax(0), total(0), status(PENDING) {}

    Payment(const std::string& method, double baseAmount)
        : method(method), amount(baseAmount), status(PENDING) {
        tax = baseAmount * TAX_RATE;
        total = baseAmount + tax;
        transactionId = generateTransactionId();
    }

    static std::string generateTransactionId() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(100000, 999999);
        return "TXN-" + std::to_string(dist(rng));
    }

    void process() {
        // Simulate payment processing
        std::cout << "  Processing " << method << " payment of $"
                  << std::fixed << std::setprecision(2) << total << "...\n";
        status = COMPLETED;
        std::cout << "  Payment APPROVED. Transaction: " << transactionId << "\n";
    }

    void refund() {
        if (status == COMPLETED) {
            status = REFUNDED;
            std::cout << "  Refund issued for $" << std::fixed
                      << std::setprecision(2) << total
                      << ". Ref: " << transactionId << "-REF\n";
        }
    }

    std::string getStatusStr() const {
        switch (status) {
            case PENDING: return "Pending";
            case COMPLETED: return "Completed";
            case FAILED: return "Failed";
            case REFUNDED: return "Refunded";
        }
        return "Unknown";
    }

    double getAmount() const { return amount; }
    double getTax() const { return tax; }
    double getTotal() const { return total; }
    std::string getMethod() const { return method; }
    std::string getTransactionId() const { return transactionId; }

    JSON toJSON() const {
        JSON j; j.type = JSON::OBJECT;
        j.obj["method"] = JSON(method);
        j.obj["subtotal"] = JSON(amount);
        j.obj["tax"] = JSON(tax);
        j.obj["total"] = JSON(total);
        j.obj["status"] = JSON(getStatusStr());
        j.obj["transactionId"] = JSON(transactionId);
        return j;
    }
};

const double Payment::TAX_RATE = 0.12;

// ═══════════════════════════════════════════════════════════════
// SECTION 5: BOOKING CLASS
// ═══════════════════════════════════════════════════════════════

class Booking {
    std::string bookingId;
    Guest guest;
    std::shared_ptr<Room> room;
    std::string checkIn;
    std::string checkOut;
    int nights;
    std::string status;
    Payment payment;
    std::string specialRequests;
    std::string createdAt;

public:
    Booking() : nights(0), status("Confirmed") {}

    Booking(const std::string& id, const Guest& g,
            std::shared_ptr<Room> r,
            const std::string& ci, const std::string& co,
            int n, const std::string& payMethod,
            const std::string& special = "")
        : bookingId(id), guest(g), room(r),
          checkIn(ci), checkOut(co), nights(n),
          status("Confirmed"),
          payment(payMethod, r->calculateTotal(n)),
          specialRequests(special) {
        createdAt = getCurrentTimestamp();
    }

    static std::string getCurrentTimestamp() {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return oss.str();
    }

    static int calculateNights(const std::string& ci, const std::string& co) {
        // Parse YYYY-MM-DD
        auto parseDate = [](const std::string& d) -> std::tm {
            std::tm tm = {};
            std::istringstream ss(d);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            return tm;
        };
        std::tm t1 = parseDate(ci);
        std::tm t2 = parseDate(co);
        auto time1 = std::mktime(&t1);
        auto time2 = std::mktime(&t2);
        double diff = std::difftime(time2, time1);
        return (int)(diff / 86400);
    }

    static bool datesOverlap(const std::string& s1, const std::string& e1,
                             const std::string& s2, const std::string& e2) {
        return s1 < e2 && e1 > s2;
    }

    void confirm() {
        status = "Confirmed";
        payment.process();
    }

    void cancel() {
        if (status != "Cancelled") {
            status = "Cancelled";
            payment.refund();
            if (room) room->setAvailable(true);
            std::cout << "  Booking " << bookingId << " cancelled.\n";
        }
    }

    void display() const {
        std::cout << "\n  ┌─ Booking: " << bookingId << " ─────────────────────\n";
        std::cout << "  │ Guest   : " << guest.getFullName() << " (" << guest.getEmail() << ")\n";
        std::cout << "  │ Room    : " << (room ? room->getName() : "N/A")
                  << " [" << (room ? room->getType() : "") << "]\n";
        std::cout << "  │ Dates   : " << checkIn << " → " << checkOut
                  << " (" << nights << " night" << (nights!=1?"s":"") << ")\n";
        std::cout << "  │ Payment : " << formatCurrency(payment.getTotal())
                  << " via " << payment.getMethod() << "\n";
        std::cout << "  │ Status  : " << status << "\n";
        std::cout << "  └──────────────────────────────────────────\n";
    }

    static std::string formatCurrency(double amount) {
        std::ostringstream oss;
        oss << "$" << std::fixed << std::setprecision(2) << amount;
        return oss.str();
    }

    // Getters
    std::string getId() const { return bookingId; }
    std::string getStatus() const { return status; }
    std::string getCheckIn() const { return checkIn; }
    std::string getCheckOut() const { return checkOut; }
    std::shared_ptr<Room> getRoom() const { return room; }
    int getNights() const { return nights; }
    const Payment& getPayment() const { return payment; }
    const Guest& getGuest() const { return guest; }

    JSON toJSON() const {
        JSON j; j.type = JSON::OBJECT;
        j.obj["id"] = JSON(bookingId);
        j.obj["guestName"] = JSON(guest.getFullName());
        j.obj["email"] = JSON(guest.getEmail());
        j.obj["phone"] = JSON(guest.getPhone());
        j.obj["idNumber"] = JSON(guest.getIdNumber());
        j.obj["guests"] = JSON((double)guest.getNumGuests());
        j.obj["roomId"] = JSON(room ? room->getId() : "");
        j.obj["roomNumber"] = JSON(room ? room->getNumber() : "");
        j.obj["roomName"] = JSON(room ? room->getName() : "");
        j.obj["roomType"] = JSON(room ? room->getType() : "");
        j.obj["roomRate"] = JSON(room ? room->getBasePrice() : 0.0);
        j.obj["checkIn"] = JSON(checkIn);
        j.obj["checkOut"] = JSON(checkOut);
        j.obj["nights"] = JSON((double)nights);
        j.obj["subtotal"] = JSON(payment.getAmount());
        j.obj["tax"] = JSON(payment.getTax());
        j.obj["total"] = JSON(payment.getTotal());
        j.obj["paymentMethod"] = JSON(payment.getMethod());
        j.obj["transactionId"] = JSON(payment.getTransactionId());
        j.obj["status"] = JSON(status);
        j.obj["specialRequests"] = JSON(specialRequests);
        j.obj["createdAt"] = JSON(createdAt);
        return j;
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 6: HOTEL MANAGER CLASS
// ═══════════════════════════════════════════════════════════════

class HotelManager {
    std::string hotelName;
    std::vector<std::shared_ptr<Room>> rooms;
    std::vector<Booking> bookings;

    // File paths
    const std::string BOOKINGS_FILE = "data/bookings.json";
    const std::string ROOMS_FILE    = "data/rooms.json";
    const std::string INPUT_FILE    = "data/pending_booking.json";
    const std::string OUTPUT_FILE   = "data/booking_result.json";

public:
    explicit HotelManager(const std::string& name) : hotelName(name) {
        initRooms();
        loadBookings();
        syncRoomAvailability();
    }

    // ─── Initialize Rooms ─────────────────────────────
    void initRooms() {
        // Standard Rooms
        auto sr1 = std::make_shared<StandardRoom>("SR101","101","Classic Standard Room", 189.0);
        sr1->addAmenity("Queen Bed"); sr1->addAmenity("City View"); sr1->addAmenity("Free WiFi");

        auto sr2 = std::make_shared<StandardRoom>("SR102","102","Garden View Standard", 199.0);
        sr2->addAmenity("King Bed"); sr2->addAmenity("Garden View"); sr2->addAmenity("Terrace");

        auto sr3 = std::make_shared<StandardRoom>("SR103","103","Twin Comfort Room", 179.0);
        sr3->addAmenity("Twin Beds"); sr3->addAmenity("Work Desk"); sr3->addAmenity("City View");

        // Deluxe Rooms
        auto dr1 = std::make_shared<DeluxeRoom>("DR201","201","Deluxe Ocean View", 349.0, true);
        dr1->addAmenity("King Bed"); dr1->addAmenity("Ocean View"); dr1->addAmenity("Jacuzzi");

        auto dr2 = std::make_shared<DeluxeRoom>("DR202","202","Deluxe Corner Suite", 389.0, false);
        dr2->addAmenity("King Bed"); dr2->addAmenity("Corner View"); dr2->addAmenity("Living Area");

        auto dr3 = std::make_shared<DeluxeRoom>("DR203","203","Deluxe Garden Terrace", 369.0, false);
        dr3->addAmenity("King Bed"); dr3->addAmenity("Private Terrace"); dr3->addAmenity("Garden Access");

        // Suites
        auto ss1 = std::make_shared<SuiteRoom>("SS301","301","The Grand Horizon Suite", 1290.0, 2, true);
        ss1->addAmenity("2 Bedrooms"); ss1->addAmenity("Private Butler"); ss1->addAmenity("Home Theatre");

        auto ss2 = std::make_shared<SuiteRoom>("SS302","302","Presidential Suite", 1890.0, 2, true);
        ss2->addAmenity("2 Bedrooms"); ss2->addAmenity("Private Pool"); ss2->addAmenity("Art Collection");

        auto ss3 = std::make_shared<SuiteRoom>("SS303","303","Horizon Sky Suite", 1490.0, 1, true);
        ss3->addAmenity("Master Bedroom"); ss3->addAmenity("Rooftop Terrace"); ss3->addAmenity("Plunge Pool");

        rooms = {sr1, sr2, sr3, dr1, dr2, dr3, ss1, ss2, ss3};
        std::cout << "  ✓ " << rooms.size() << " rooms initialized.\n";
    }

    // ─── Room Lookup ──────────────────────────────────
    std::shared_ptr<Room> findRoom(const std::string& roomId) {
        for (auto& r : rooms)
            if (r->getId() == roomId) return r;
        return nullptr;
    }

    // ─── Generate Unique Booking ID ───────────────────
    std::string generateBookingId() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(1000, 9999);
        auto t = std::time(nullptr);
        std::ostringstream oss;
        oss << "GH-" << std::hex << std::uppercase << (t & 0xFFFF)
            << "-" << dist(rng);
        return oss.str();
    }

    // ─── Double Booking Check ─────────────────────────
    bool isDoubleBooked(const std::string& roomId,
                        const std::string& ci, const std::string& co,
                        const std::string& excludeId = "") const {
        for (const auto& b : bookings) {
            if (b.getStatus() == "Cancelled") continue;
            if (!excludeId.empty() && b.getId() == excludeId) continue;
            if (b.getRoom() && b.getRoom()->getId() != roomId) continue;
            if (Booking::datesOverlap(ci, co, b.getCheckIn(), b.getCheckOut()))
                return true;
        }
        return false;
    }

    // ─── Process Booking from JSON ────────────────────
    JSON processBookingRequest(const JSON& req) {
        JSON result; result.type = JSON::OBJECT;

        std::cout << "\n  Processing booking request...\n";

        // 1. Extract and validate guest info
        Guest guest = Guest::fromJSON(req);
        if (!guest.isValid()) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Invalid guest information. Please check all required fields.");
            return result;
        }

        // 2. Find room
        std::string roomId = req.getString("roomId");
        auto room = findRoom(roomId);
        if (!room) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Room not found: " + roomId);
            return result;
        }

        // 3. Validate availability
        if (!room->isAvailable()) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Room " + room->getNumber() + " is not available.");
            return result;
        }

        // 4. Validate dates
        std::string ci = req.getString("checkIn");
        std::string co = req.getString("checkOut");
        if (ci.empty() || co.empty() || ci >= co) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Invalid dates. Check-out must be after check-in.");
            return result;
        }

        int nights = Booking::calculateNights(ci, co);
        if (nights < 1) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Minimum stay is 1 night.");
            return result;
        }

        // 5. Check double booking
        if (isDoubleBooked(roomId, ci, co)) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Room " + room->getNumber()
                + " is already booked for " + ci + " to " + co + ". Please select different dates.");
            return result;
        }

        // 6. Validate guest capacity
        if (guest.getNumGuests() > room->getMaxGuests()) {
            result.obj["success"] = JSON(false);
            result.obj["error"] = JSON("Room " + room->getNumber()
                + " has max capacity of " + std::to_string(room->getMaxGuests()) + " guests.");
            return result;
        }

        // 7. Create booking
        std::string payMethod = req.getString("paymentMethod", "Credit Card");
        std::string special   = req.getString("specialRequests");
        std::string bookingId = generateBookingId();

        Booking booking(bookingId, guest, room, ci, co, nights, payMethod, special);
        booking.confirm();
        room->setAvailable(false);

        bookings.push_back(booking);
        saveBookings();

        booking.display();

        // 8. Build success response
        result.obj["success"] = JSON(true);
        result.obj["booking"] = booking.toJSON();
        result.obj["message"] = JSON("Booking confirmed. "
            + std::to_string(nights) + " night(s) in "
            + room->getName() + ". Total: "
            + Booking::formatCurrency(booking.getPayment().getTotal()));
        result.obj["timestamp"] = JSON(Booking::getCurrentTimestamp());
        result.obj["type"] = JSON("BOOKING_RESULT");
        return result;
    }

    // ─── Cancel Booking ───────────────────────────────
    JSON cancelBooking(const std::string& bookingId) {
        JSON result; result.type = JSON::OBJECT;
        for (auto& b : bookings) {
            if (b.getId() == bookingId) {
                b.cancel();
                saveBookings();
                syncRoomAvailability();
                result.obj["success"] = JSON(true);
                result.obj["message"] = JSON("Booking " + bookingId + " cancelled.");
                result.obj["type"] = JSON("CANCEL_RESULT");
                return result;
            }
        }
        result.obj["success"] = JSON(false);
        result.obj["error"] = JSON("Booking not found: " + bookingId);
        return result;
    }

    // ─── Sync Room Availability from Bookings ─────────
    void syncRoomAvailability() {
        // Reset all to available
        for (auto& r : rooms) r->setAvailable(true);
        // Mark booked rooms
        for (const auto& b : bookings) {
            if (b.getStatus() != "Cancelled" && b.getRoom()) {
                auto room = findRoom(b.getRoom()->getId());
                if (room) {
                    // Only mark unavailable if checkout hasn't passed
                    // (simplified: mark all non-cancelled as unavailable)
                    room->setAvailable(false);
                }
            }
        }
    }

    // ─── File I/O ─────────────────────────────────────
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    void writeFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (file.is_open()) file << content;
    }

    void loadBookings() {
        std::string content = readFile(BOOKINGS_FILE);
        if (content.empty()) {
            std::cout << "  (No existing bookings found — starting fresh)\n";
            return;
        }
        try {
            JSON data = JSON::parse(content);
            if (data.type == JSON::OBJECT) {
                auto& bArr = data.obj["bookings"];
                for (const auto& bj : bArr.arr) {
                    // Reconstruct lightweight booking record for collision checking
                    // (Full reconstruction would need Room refs — here we use a minimal form)
                    std::string rid = bj.getString("roomId");
                    auto room = findRoom(rid);
                    if (room) {
                        Guest g(bj.getString("firstName", bj.getString("guestName")),
                                "",
                                bj.getString("email"),
                                bj.getString("phone"),
                                bj.getString("idNumber"),
                                (int)bj.getNumber("guests", 1));
                        std::string ci = bj.getString("checkIn");
                        std::string co = bj.getString("checkOut");
                        int n = Booking::calculateNights(ci, co);
                        // Reconstruct booking (simplified)
                        Booking b(bj.getString("id"), g, room, ci, co, n,
                                  bj.getString("paymentMethod", "Card"));
                        bookings.push_back(b);
                    }
                }
            }
            std::cout << "  ✓ Loaded " << bookings.size() << " existing booking(s).\n";
        } catch (...) {
            std::cout << "  (Could not parse bookings.json)\n";
        }
    }

    void saveBookings() {
        JSON root; root.type = JSON::OBJECT;
        root.obj["hotel"] = JSON(hotelName);
        root.obj["savedAt"] = JSON(Booking::getCurrentTimestamp());
        root.obj["totalBookings"] = JSON((double)bookings.size());

        JSON bArr; bArr.type = JSON::ARRAY;
        for (const auto& b : bookings) bArr.arr.push_back(b.toJSON());
        root.obj["bookings"] = bArr;

        writeFile(BOOKINGS_FILE, root.serialize());
        std::cout << "  ✓ Bookings saved to " << BOOKINGS_FILE << "\n";
    }

    void saveRooms() {
        JSON root; root.type = JSON::OBJECT;
        root.obj["hotel"] = JSON(hotelName);
        root.obj["savedAt"] = JSON(Booking::getCurrentTimestamp());

        JSON rArr; rArr.type = JSON::ARRAY;
        for (const auto& r : rooms) rArr.arr.push_back(r->toJSON());
        root.obj["rooms"] = rArr;

        writeFile(ROOMS_FILE, root.serialize());
        std::cout << "  ✓ Rooms saved to " << ROOMS_FILE << "\n";
    }

    // ─── Display Functions ────────────────────────────
    void displayAllRooms() const {
        std::cout << "\n  ╔═════════════════════════════════════╗\n";
        std::cout << "  ║        ROOM INVENTORY               ║\n";
        std::cout << "  ╚═════════════════════════════════════╝\n";
        for (const auto& r : rooms) r->display();
    }

    void displayAllBookings() const {
        std::cout << "\n  ╔═════════════════════════════════════╗\n";
        std::cout << "  ║        ALL RESERVATIONS             ║\n";
        std::cout << "  ╚═════════════════════════════════════╝\n";
        if (bookings.empty()) {
            std::cout << "  (No bookings on record)\n"; return;
        }
        for (const auto& b : bookings) b.display();
    }

    void displaySummary() const {
        int confirmed = 0; double revenue = 0;
        for (const auto& b : bookings) {
            if (b.getStatus() == "Confirmed") {
                confirmed++;
                revenue += b.getPayment().getTotal();
            }
        }
        std::cout << "\n  ══ HOTEL SUMMARY ══════════════════════\n";
        std::cout << "  Hotel    : " << hotelName << "\n";
        std::cout << "  Rooms    : " << rooms.size() << " total\n";
        std::cout << "  Bookings : " << bookings.size() << " total / "
                  << confirmed << " confirmed\n";
        std::cout << "  Revenue  : " << Booking::formatCurrency(revenue) << "\n";
        std::cout << "  ═══════════════════════════════════════\n";
    }

    // ─── Main Processing Loop ─────────────────────────
    void run() {
        std::cout << "\n  ╔══════════════════════════════════════════╗\n";
        std::cout << "  ║  " << hotelName << " — Backend System     ║\n";
        std::cout << "  ╚══════════════════════════════════════════╝\n\n";

        // Save rooms inventory
        saveRooms();

        bool running = true;
        while (running) {
            std::cout << "\n  ┌─ MENU ─────────────────────────────────\n";
            std::cout << "  │ 1. Process Pending Booking (from file)\n";
            std::cout << "  │ 2. Make New Booking (interactive)\n";
            std::cout << "  │ 3. Cancel Booking\n";
            std::cout << "  │ 4. View All Rooms\n";
            std::cout << "  │ 5. View All Reservations\n";
            std::cout << "  │ 6. Hotel Summary\n";
            std::cout << "  │ 7. Exit\n";
            std::cout << "  └────────────────────────────────────────\n";
            std::cout << "  Choice: ";

            int choice;
            if (!(std::cin >> choice)) break;
            std::cin.ignore();

            switch (choice) {
                case 1: processFileBooking(); break;
                case 2: interactiveBooking(); break;
                case 3: interactiveCancellation(); break;
                case 4: displayAllRooms(); break;
                case 5: displayAllBookings(); break;
                case 6: displaySummary(); break;
                case 7: running = false; break;
                default: std::cout << "  Invalid choice.\n";
            }
        }
        std::cout << "\n  Goodbye from " << hotelName << "!\n\n";
    }

    // ─── File-Based Booking ───────────────────────────
    void processFileBooking() {
        std::cout << "\n  Reading " << INPUT_FILE << "...\n";
        std::string content = readFile(INPUT_FILE);
        if (content.empty()) {
            std::cout << "  No pending booking found. Frontend must write to " << INPUT_FILE << " first.\n";
            return;
        }

        JSON req = JSON::parse(content);
        JSON result = processBookingRequest(req);
        writeFile(OUTPUT_FILE, result.serialize());
        std::cout << "  Result written to " << OUTPUT_FILE << "\n";

        if (result.getBool("success")) {
            // Clear processed input
            writeFile(INPUT_FILE, "{}");
        }
    }

    // ─── Interactive Booking (CLI) ─────────────────────
    void interactiveBooking() {
        std::cout << "\n  ── NEW BOOKING ──────────────────────────\n";

        // Display available rooms
        std::cout << "\n  Available Rooms:\n";
        for (const auto& r : rooms) {
            if (r->isAvailable()) {
                std::cout << "  [" << r->getId() << "] " << r->getType()
                          << " - " << r->getName()
                          << " ($" << std::fixed << std::setprecision(2)
                          << r->getBasePrice() << "/nt)\n";
            }
        }

        // Collect input
        auto prompt = [](const std::string& label) {
            std::cout << "  " << label << ": ";
            std::string val; std::getline(std::cin, val);
            return val;
        };

        std::string fn = prompt("First Name");
        std::string ln = prompt("Last Name");
        std::string em = prompt("Email");
        std::string ph = prompt("Phone");
        std::string id = prompt("ID Number");
        std::string gn = prompt("Number of Guests");
        std::string ri = prompt("Room ID");
        std::string ci = prompt("Check-In (YYYY-MM-DD)");
        std::string co = prompt("Check-Out (YYYY-MM-DD)");
        std::string pm = prompt("Payment Method (Credit Card/Debit Card/Cash)");
        std::string sr = prompt("Special Requests (optional)");

        // Build JSON request
        JSON req; req.type = JSON::OBJECT;
        req.obj["firstName"] = JSON(fn);
        req.obj["lastName"]  = JSON(ln);
        req.obj["email"]     = JSON(em);
        req.obj["phone"]     = JSON(ph);
        req.obj["idNumber"]  = JSON(id);
        req.obj["guests"]    = JSON(std::stod(gn.empty() ? "1" : gn));
        req.obj["roomId"]    = JSON(ri);
        req.obj["checkIn"]   = JSON(ci);
        req.obj["checkOut"]  = JSON(co);
        req.obj["paymentMethod"] = JSON(pm);
        req.obj["specialRequests"] = JSON(sr);

        JSON result = processBookingRequest(req);
        writeFile(OUTPUT_FILE, result.serialize());

        if (result.getBool("success")) {
            std::cout << "\n  ✓ " << result.getString("message") << "\n";
        } else {
            std::cout << "\n  ✗ Error: " << result.getString("error") << "\n";
        }
    }

    // ─── Interactive Cancellation ─────────────────────
    void interactiveCancellation() {
        std::cout << "\n  ── CANCEL BOOKING ───────────────────────\n";
        displayAllBookings();
        std::cout << "  Enter Booking ID to cancel: ";
        std::string id; std::getline(std::cin, id);
        JSON result = cancelBooking(id);
        if (result.getBool("success")) {
            std::cout << "  ✓ " << result.getString("message") << "\n";
        } else {
            std::cout << "  ✗ " << result.getString("error") << "\n";
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// SECTION 7: MAIN ENTRY POINT
// ═══════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "  ████████████████████████████████████████████\n";
    std::cout << "  █                                          █\n";
    std::cout << "  █     GRAND HORIZON HOTEL MANAGEMENT       █\n";
    std::cout << "  █          C++ Backend System              █\n";
    std::cout << "  █                                          █\n";
    std::cout << "  ████████████████████████████████████████████\n\n";

    HotelManager hotel("Grand Horizon Hotel");
    hotel.run();
    return 0;
}
