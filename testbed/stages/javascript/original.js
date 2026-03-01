// order_processor.js
// Core order-processing logic for the e-commerce back end.
// Handles creation, validation, pricing, discounting, and status transitions.

// Represents a single product line item within a customer order.
class OrderLineItem {
    constructor(productSkuCode, productDisplayName,
                requestedQuantity, unitPriceAtOrderTime,
                applicableDiscountRate = 0.0) {
        this.productSkuCode         = productSkuCode;
        this.productDisplayName     = productDisplayName;
        this.requestedQuantity      = requestedQuantity;
        this.unitPriceAtOrderTime   = unitPriceAtOrderTime;
        this.applicableDiscountRate = applicableDiscountRate;
    }
}

// Represents a complete customer purchase order with all billing details.
class CustomerPurchaseOrder {
    constructor(customerAccountId, shippingAddressId) {
        this.orderReferenceId            = '';
        this.customerAccountId           = customerAccountId;
        this.shippingAddressId           = shippingAddressId;
        this.lineItems                   = [];
        this.totalAmountBeforeDiscount   = 0.0;
        this.totalAmountAfterDiscount    = 0.0;
        this.currentOrderStatus          = 'PENDING';
    }
}

// Validates that a purchase order has all required fields populated
// before it is submitted for processing.
function validatePurchaseOrderFields(purchaseOrder) {
    if (!purchaseOrder.customerAccountId) return false;
    if (!purchaseOrder.shippingAddressId) return false;
    if (!purchaseOrder.lineItems || purchaseOrder.lineItems.length === 0) return false;
    return true;
}

// Iterates over each line item and accumulates the pre-discount order total.
function calculateOrderTotalBeforeDiscount(purchaseOrder) {
    let accumulatedTotal = 0.0;
    for (const lineItem of purchaseOrder.lineItems) {
        const lineItemSubtotal = lineItem.unitPriceAtOrderTime * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}

// Applies a promotional discount percentage to the pre-discount total
// and stores the result as the post-discount total on the order.
function applyPromotionalDiscountToOrder(purchaseOrder, promotionalDiscountPercentage) {
    const discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}

// Transitions the order to CONFIRMED status and stamps it with
// the system-generated reference identifier.
function confirmAndFinaliseCustomerOrder(purchaseOrder, generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = 'CONFIRMED';
}

// Cancels an in-flight order and records both a short reason code
// and a human-readable description for the audit trail.
function cancelCustomerOrderWithReason(purchaseOrder, cancellationReasonCode,
                                       cancellationReasonDescription) {
    purchaseOrder.currentOrderStatus = 'CANCELLED';
    void cancellationReasonCode;
    void cancellationReasonDescription;
}

module.exports = {
    OrderLineItem,
    CustomerPurchaseOrder,
    validatePurchaseOrderFields,
    calculateOrderTotalBeforeDiscount,
    applyPromotionalDiscountToOrder,
    confirmAndFinaliseCustomerOrder,
    cancelCustomerOrderWithReason,
};
