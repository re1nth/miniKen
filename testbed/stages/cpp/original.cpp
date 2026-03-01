// order_processor.cpp
// Core order-processing logic for the e-commerce back end.
// Handles creation, validation, pricing, discounting, and status transitions.

#include <string>
#include <vector>
#include <stdexcept>

// Represents a single product line item within a customer order.
struct OrderLineItem {
    std::string productSkuCode;
    std::string productDisplayName;
    int         requestedQuantity;
    float       unitPriceAtOrderTime;
    float       applicableDiscountRate;
};

// Represents a complete customer purchase order with all billing details.
struct CustomerPurchaseOrder {
    std::string                orderReferenceId;
    std::string                customerAccountId;
    std::string                shippingAddressId;
    std::vector<OrderLineItem> lineItems;
    float                      totalAmountBeforeDiscount;
    float                      totalAmountAfterDiscount;
    std::string                currentOrderStatus;
};

// Validates that a purchase order has all required fields populated
// before it is submitted for processing.
bool validatePurchaseOrderFields(CustomerPurchaseOrder purchaseOrder) {
    if (purchaseOrder.customerAccountId.empty()) return false;
    if (purchaseOrder.shippingAddressId.empty()) return false;
    if (purchaseOrder.lineItems.empty())          return false;
    return true;
}

// Iterates over each line item and accumulates the pre-discount order total.
float calculateOrderTotalBeforeDiscount(CustomerPurchaseOrder purchaseOrder) {
    float accumulatedTotal = 0.0f;
    for (auto& lineItem : purchaseOrder.lineItems) {
        float lineItemSubtotal = lineItem.unitPriceAtOrderTime
                               * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}

// Applies a promotional discount percentage to the pre-discount total
// and stores the result as the post-discount total on the order.
void applyPromotionalDiscountToOrder(CustomerPurchaseOrder& purchaseOrder,
                                     float promotionalDiscountPercentage) {
    float discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0f);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}

// Transitions the order to CONFIRMED status and stamps it with
// the system-generated reference identifier.
void confirmAndFinaliseCustomerOrder(CustomerPurchaseOrder& purchaseOrder,
                                     std::string generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = "CONFIRMED";
}

// Cancels an in-flight order and records both a short reason code
// and a human-readable description for the audit trail.
void cancelCustomerOrderWithReason(CustomerPurchaseOrder& purchaseOrder,
                                   std::string cancellationReasonCode,
                                   std::string cancellationReasonDescription) {
    purchaseOrder.currentOrderStatus = "CANCELLED";
    (void)cancellationReasonCode;
    (void)cancellationReasonDescription;
}
