// OrderProcessor.java
// Core order-processing logic for the e-commerce back end.
// Handles creation, validation, pricing, discounting, and status transitions.

import java.util.List;
import java.util.ArrayList;

// Represents a single product line item within a customer order.
class OrderLineItem {
    String productSkuCode;
    String productDisplayName;
    int    requestedQuantity;
    float  unitPriceAtOrderTime;
    float  applicableDiscountRate;

    OrderLineItem(String productSkuCode, String productDisplayName,
                  int requestedQuantity, float unitPriceAtOrderTime,
                  float applicableDiscountRate) {
        this.productSkuCode        = productSkuCode;
        this.productDisplayName    = productDisplayName;
        this.requestedQuantity     = requestedQuantity;
        this.unitPriceAtOrderTime  = unitPriceAtOrderTime;
        this.applicableDiscountRate = applicableDiscountRate;
    }
}

// Represents a complete customer purchase order with all billing details.
class CustomerPurchaseOrder {
    String             orderReferenceId;
    String             customerAccountId;
    String             shippingAddressId;
    List<OrderLineItem> lineItems;
    float              totalAmountBeforeDiscount;
    float              totalAmountAfterDiscount;
    String             currentOrderStatus;

    CustomerPurchaseOrder(String customerAccountId, String shippingAddressId) {
        this.customerAccountId = customerAccountId;
        this.shippingAddressId = shippingAddressId;
        this.lineItems         = new ArrayList<>();
        this.currentOrderStatus = "PENDING";
    }
}

// Validates that a purchase order has all required fields populated
// before it is submitted for processing.
boolean validatePurchaseOrderFields(CustomerPurchaseOrder purchaseOrder) {
    if (purchaseOrder.customerAccountId == null || purchaseOrder.customerAccountId.isEmpty())
        return false;
    if (purchaseOrder.shippingAddressId == null || purchaseOrder.shippingAddressId.isEmpty())
        return false;
    if (purchaseOrder.lineItems == null || purchaseOrder.lineItems.isEmpty())
        return false;
    return true;
}

// Iterates over each line item and accumulates the pre-discount order total.
float calculateOrderTotalBeforeDiscount(CustomerPurchaseOrder purchaseOrder) {
    float accumulatedTotal = 0.0f;
    for (OrderLineItem lineItem : purchaseOrder.lineItems) {
        float lineItemSubtotal = lineItem.unitPriceAtOrderTime * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}

// Applies a promotional discount percentage to the pre-discount total
// and stores the result as the post-discount total on the order.
void applyPromotionalDiscountToOrder(CustomerPurchaseOrder purchaseOrder,
                                     float promotionalDiscountPercentage) {
    float discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0f);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}

// Transitions the order to CONFIRMED status and stamps it with
// the system-generated reference identifier.
void confirmAndFinaliseCustomerOrder(CustomerPurchaseOrder purchaseOrder,
                                     String generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = "CONFIRMED";
}

// Cancels an in-flight order and records both a short reason code
// and a human-readable description for the audit trail.
void cancelCustomerOrderWithReason(CustomerPurchaseOrder purchaseOrder,
                                   String cancellationReasonCode,
                                   String cancellationReasonDescription) {
    purchaseOrder.currentOrderStatus = "CANCELLED";
}
