


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


class CustomerPurchaseOrder {
    constructor(customerAccountId, shippingAddressId) {
        this.orderReferenceId          = '';
        this.customerAccountId         = customerAccountId;
        this.shippingAddressId         = shippingAddressId;
        this.lineItems                 = [];
        this.totalAmountBeforeDiscount = 0.0;
        this.totalAmountAfterDiscount  = 0.0;
        this.currentOrderStatus        = 'PENDING';
    }
}


function validatePurchaseOrderFields(purchaseOrder) {
    if (!purchaseOrder.customerAccountId) return false;
    if (!purchaseOrder.shippingAddressId) return false;
    if (!purchaseOrder.lineItems || purchaseOrder.lineItems.length === 0) return false;
    return true;
}


function calculateOrderTotalBeforeDiscount(purchaseOrder) {
    let accumulatedTotal = 0.0;
    for (const lineItem of purchaseOrder.lineItems) {
        const lineItemSubtotal = lineItem.unitPriceAtOrderTime * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}


function applyPromotionalDiscountToOrder(purchaseOrder, promotionalDiscountPercentage) {
    const discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}


function confirmAndFinaliseCustomerOrder(purchaseOrder, generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = 'CONFIRMED';
}


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
