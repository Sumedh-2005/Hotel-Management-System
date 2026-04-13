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
