



#include <string>
#include <vector>
#include <stdexcept>


struct OrderLineItem {
    std::string productSkuCode;
    std::string productDisplayName;
    int         requestedQuantity;
    float       unitPriceAtOrderTime;
    float       applicableDiscountRate;
};


struct CustomerPurchaseOrder {
    std::string                orderReferenceId;
    std::string                customerAccountId;
    std::string                shippingAddressId;
    std::vector<OrderLineItem> lineItems;
    float                      totalAmountBeforeDiscount;
    float                      totalAmountAfterDiscount;
    std::string                currentOrderStatus;
};



bool validatePurchaseOrderFields(CustomerPurchaseOrder purchaseOrder) {
    if (purchaseOrder.customerAccountId.empty()) return false;
    if (purchaseOrder.shippingAddressId.empty()) return false;
    if (purchaseOrder.lineItems.empty())          return false;
    return true;
}


float calculateOrderTotalBeforeDiscount(CustomerPurchaseOrder purchaseOrder) {
    float accumulatedTotal = 0.0f;
    for (auto& lineItem : purchaseOrder.lineItems) {
        float lineItemSubtotal = lineItem.unitPriceAtOrderTime
                               * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}



void applyPromotionalDiscountToOrder(CustomerPurchaseOrder& purchaseOrder,
                                     float promotionalDiscountPercentage) {
    float discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0f);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}



void confirmAndFinaliseCustomerOrder(CustomerPurchaseOrder& purchaseOrder,
                                     std::string generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = "CONFIRMED";
}



void cancelCustomerOrderWithReason(CustomerPurchaseOrder& purchaseOrder,
                                   std::string cancellationReasonCode,
                                   std::string cancellationReasonDescription) {
    purchaseOrder.currentOrderStatus = "CANCELLED";
    (void)cancellationReasonCode;
    (void)cancellationReasonDescription;
}
