// benchmark.cpp
// Token-performance benchmark: compresses representative C++ code samples and
// reports side-by-side token / character reduction statistics.
//
// "Token" here is approximated as whitespace-separated words — a reasonable
// proxy for LLM sub-word tokenization.
#include "sessionStore/sessionMapObject.h"
#include "parser/querySquasher.h"
#include "parser/cppProfile.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/types.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::string writeTempCpp(const std::string& content) {
    char tmpl[] = "/tmp/mk_bench_XXXXXX.cpp";
    int fd = mkstemps(tmpl, 4);
    if (fd < 0) throw std::runtime_error("mkstemps failed");
    ::write(fd, content.c_str(), content.size());
    ::close(fd);
    return std::string(tmpl);
}

static int countTokens(const std::string& text) {
    int count = 0;
    bool inTok = false;
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            inTok = false;
        } else {
            if (!inTok) { ++count; inTok = true; }
        }
    }
    return count;
}

static std::string truncate(const std::string& s, std::size_t maxLen = 120) {
    if (s.size() <= maxLen) return s;
    return s.substr(0, maxLen) + " ...";
}

struct Sample {
    std::string label;
    std::string source;
};

// ─── Code fixtures ────────────────────────────────────────────────────────────

static const std::vector<Sample> kSamples = {

// ── Sample 1: single utility function ────────────────────────────────────────
{ "single_utility_function", R"cpp(
// Computes the total price after applying a percentage discount
float calculateDiscountedTotalPrice(int unitPrice, int quantity,
                                    float discountPercentage) {
    float discountAmount = unitPrice * (discountPercentage / 100.0f);
    float discountedUnitPrice = unitPrice - discountAmount;
    return discountedUnitPrice * quantity;
}
)cpp"},

// ── Sample 2: small class with methods ───────────────────────────────────────
{ "small_class_with_methods", R"cpp(
// Represents a customer's shopping cart
class ShoppingCartManager {
public:
    // Adds a product to the cart
    void addProductToCart(ProductItem product, int requestedQuantity);
    // Removes a product from the cart
    void removeProductFromCart(ProductItem product);
    // Returns the total price of all items
    float calculateCartTotalPrice() const;
    // Applies a discount voucher to the cart
    void applyDiscountVoucher(DiscountVoucher discountVoucher);
    // Clears all items from the cart
    void clearShoppingCart();

private:
    std::vector<CartLineItem> cartLineItems_;
    float cartTotalPrice_;
    DiscountVoucher activeDiscountVoucher_;
};
)cpp"},

// ── Sample 3: two collaborating classes ──────────────────────────────────────
{ "two_collaborating_classes", R"cpp(
// Processes customer orders end-to-end
class CustomerOrderProcessor {
public:
    CustomerOrderProcessor(DatabaseConnection databaseConnection);

    // Processes a single customer order
    bool processCustomerOrder(CustomerOrder customerOrder);

    // Validates all items in the order
    void validateOrderLineItems(CustomerOrder customerOrder);

    // Applies any applicable customer discounts
    void applyCustomerLoyaltyDiscounts(CustomerAccount customerAccount,
                                       CustomerOrder customerOrder);

    // Sends a confirmation email to the customer
    void sendOrderConfirmationEmail(CustomerAccount customerAccount,
                                    CustomerOrder customerOrder);

private:
    DatabaseConnection databaseConnection_;
    EmailServiceClient emailServiceClient_;
};

// Tracks product inventory levels
class ProductInventoryTracker {
public:
    // Checks whether a product has sufficient stock
    bool checkProductAvailability(ProductItem productItem, int requestedQuantity);

    // Decrements inventory after a sale
    void decrementInventoryAfterSale(ProductItem productItem, int soldQuantity);

    // Triggers a restock notification when stock falls below threshold
    void triggerLowStockRestockNotification(ProductItem productItem,
                                            int currentStockLevel);

private:
    InventoryDatabase inventoryDatabase_;
    NotificationService notificationService_;
};
)cpp"},

// ── Sample 4: authentication module with several commented functions ──────────
{ "authentication_module", R"cpp(
// Authenticates a user with username and password credentials
bool authenticateUserWithCredentials(std::string usernameInput,
                                     std::string passwordInput) {
    UserAccountRecord userAccountRecord = findUserAccountByUsername(usernameInput);
    if (userAccountRecord.isEmpty()) {
        recordUnknownUserLoginAttempt(usernameInput);
        return false;
    }
    if (!validatePasswordAgainstStoredHash(passwordInput,
                                           userAccountRecord.passwordHash)) {
        recordFailedPasswordLoginAttempt(usernameInput);
        return false;
    }
    updateLastSuccessfulLoginTimestamp(userAccountRecord);
    return true;
}

// Validates a plaintext password against a stored bcrypt hash
bool validatePasswordAgainstStoredHash(std::string plaintextPassword,
                                        std::string storedPasswordHash) {
    std::string computedPasswordHash = computeBcryptHash(plaintextPassword);
    return computedPasswordHash == storedPasswordHash;
}

// Looks up a user account record by username
UserAccountRecord findUserAccountByUsername(std::string usernameInput) {
    DatabaseSelectQuery selectQuery = buildSelectQuery("user_accounts",
                                                       "username",
                                                       usernameInput);
    return executeDatabaseSelectQuery(selectQuery);
}

// Records a failed login attempt for security auditing
void recordFailedPasswordLoginAttempt(std::string usernameInput) {
    SecurityAuditEvent securityAuditEvent =
        createSecurityAuditEvent("FAILED_PASSWORD_LOGIN", usernameInput);
    persistSecurityAuditEventToLog(securityAuditEvent);
    incrementFailedLoginAttemptCounter(usernameInput);
}

// Records an unknown user login attempt
void recordUnknownUserLoginAttempt(std::string usernameInput) {
    SecurityAuditEvent securityAuditEvent =
        createSecurityAuditEvent("UNKNOWN_USER_LOGIN", usernameInput);
    persistSecurityAuditEventToLog(securityAuditEvent);
}

// Updates the last successful login timestamp on the user record
void updateLastSuccessfulLoginTimestamp(UserAccountRecord userAccountRecord) {
    userAccountRecord.lastSuccessfulLoginTimestamp = getCurrentSystemTimestamp();
    persistUpdatedUserAccountRecord(userAccountRecord);
}

// Computes a bcrypt hash of the given plaintext password
std::string computeBcryptHash(std::string plaintextPassword) {
    BcryptHasher bcryptHasher;
    bcryptHasher.setCostFactor(kDefaultBcryptCostFactor);
    return bcryptHasher.hash(plaintextPassword);
}
)cpp"},

// ── Sample 5: payment processing pipeline ────────────────────────────────────
{ "payment_processing_pipeline", R"cpp(
// Orchestrates the complete payment processing pipeline
class PaymentProcessingOrchestrator {
public:
    PaymentProcessingOrchestrator(PaymentGatewayClient paymentGatewayClient,
                                   FraudDetectionService fraudDetectionService,
                                   TransactionLedgerDatabase transactionLedgerDatabase,
                                   NotificationDispatcher notificationDispatcher);

    // Processes a payment request end-to-end
    PaymentProcessingResult processPaymentRequest(
        PaymentRequest paymentRequest,
        CustomerBillingProfile customerBillingProfile);

    // Validates that the payment request has all required fields
    bool validatePaymentRequestFields(PaymentRequest paymentRequest);

    // Checks the customer's billing profile for active flags
    bool checkCustomerBillingProfileStatus(
        CustomerBillingProfile customerBillingProfile);

    // Runs the payment through the fraud detection service
    FraudAssessmentResult assessPaymentFraudRisk(
        PaymentRequest paymentRequest,
        CustomerBillingProfile customerBillingProfile);

    // Submits the payment to the payment gateway
    GatewaySubmissionResult submitPaymentToGateway(
        PaymentRequest paymentRequest);

    // Records the transaction in the ledger regardless of outcome
    void recordTransactionInLedger(PaymentRequest paymentRequest,
                                    GatewaySubmissionResult gatewaySubmissionResult);

    // Dispatches success or failure notification to the customer
    void dispatchPaymentOutcomeNotification(
        CustomerBillingProfile customerBillingProfile,
        PaymentProcessingResult paymentProcessingResult);

    // Handles a gateway timeout by scheduling a retry
    void handleGatewayTimeoutWithRetryScheduling(
        PaymentRequest paymentRequest,
        int retryAttemptNumber);

private:
    PaymentGatewayClient       paymentGatewayClient_;
    FraudDetectionService      fraudDetectionService_;
    TransactionLedgerDatabase  transactionLedgerDatabase_;
    NotificationDispatcher     notificationDispatcher_;
    int                        maxRetryAttempts_;
};
)cpp"},

};  // kSamples

// ─── Reporting ────────────────────────────────────────────────────────────────

static void printDivider(int width = 70) {
    std::cout << std::string(width, '=') << "\n";
}

int main() {
    printDivider();
    std::cout << "  miniKen Token Performance Benchmark\n";
    printDivider();

    double totalTokenReduction = 0.0;
    double totalCharReduction  = 0.0;
    int    successCount        = 0;

    for (const auto& sample : kSamples) {
        std::cout << "\n[Sample: " << sample.label << "]\n";

        std::string path;
        std::string compressed;
        try {
            path = writeTempCpp(sample.source);
            SessionMapObject session;
            compressed = QuerySquasher::apply(session, "bench", path, makeCppProfile());
        } catch (std::exception& e) {
            std::cout << "  ERROR: " << e.what() << "\n";
            continue;
        }

        int origTokens = countTokens(sample.source);
        int compTokens = countTokens(compressed);
        int origChars  = static_cast<int>(sample.source.size());
        int compChars  = static_cast<int>(compressed.size());

        double tokenPct = origTokens > 0
            ? 100.0 * (origTokens - compTokens) / origTokens
            : 0.0;
        double charPct  = origChars > 0
            ? 100.0 * (origChars - compChars) / origChars
            : 0.0;

        totalTokenReduction += tokenPct;
        totalCharReduction  += charPct;
        ++successCount;

        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  Original  : " << std::setw(5) << origTokens
                  << " tokens  " << std::setw(6) << origChars  << " chars\n";
        std::cout << "  Compressed: " << std::setw(5) << compTokens
                  << " tokens  " << std::setw(6) << compChars  << " chars\n";
        std::cout << "  Reduction : " << std::setw(5) << tokenPct
                  << "% tokens  " << std::setw(5)  << charPct   << "% chars\n";

        // Show a side-by-side snippet (first non-empty line of each).
        // Return the first non-empty line, stripped of leading whitespace.
        auto firstLine = [](const std::string& s) -> std::string {
            std::size_t pos = 0;
            while (pos < s.size()) {
                std::size_t nl = s.find('\n', pos);
                std::size_t end = (nl == std::string::npos) ? s.size() : nl;
                std::string line = s.substr(pos, end - pos);
                auto q = line.find_first_not_of(" \t\r");
                if (q != std::string::npos) return line.substr(q);
                pos = end + 1;
            }
            return {};
        };
        std::cout << "  Original  snippet: " << truncate(firstLine(sample.source), 60) << "\n";
        std::cout << "  Compressed snippet: " << truncate(firstLine(compressed),    60) << "\n";
    }

    if (successCount > 0) {
        std::cout << "\n";
        printDivider();
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "  Summary across " << successCount << " samples:\n";
        std::cout << "  Avg token reduction : "
                  << (totalTokenReduction / successCount) << "%\n";
        std::cout << "  Avg char reduction  : "
                  << (totalCharReduction  / successCount) << "%\n";
        printDivider();
    }

    return 0;
}
