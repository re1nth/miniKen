


import java.util.List;
import java.util.ArrayList;

class OrderLineItem {
    String productSkuCode;
    String productDisplayName;
    int    requestedQuantity;
    float  unitPriceAtOrderTime;
    float  applicableDiscountRate;

    OrderLineItem(String productSkuCode, String productDisplayName,
                  int requestedQuantity, float unitPriceAtOrderTime,
                  float applicableDiscountRate) {
        this.productSkuCode         = productSkuCode;
        this.productDisplayName     = productDisplayName;
        this.requestedQuantity      = requestedQuantity;
        this.unitPriceAtOrderTime   = unitPriceAtOrderTime;
        this.applicableDiscountRate = applicableDiscountRate;
    }
}

class CustomerPurchaseOrder {
    String             orderReferenceId;
    String             customerAccountId;
    String             shippingAddressId;
    List<OrderLineItem> lineItems;
    float              totalAmountBeforeDiscount;
    float              totalAmountAfterDiscount;
    String             currentOrderStatus;

    CustomerPurchaseOrder(String customerAccountId, String shippingAddressId) {
        this.customerAccountId  = customerAccountId;
        this.shippingAddressId  = shippingAddressId;
        this.lineItems          = new ArrayList<>();
        this.currentOrderStatus = "PENDING";
    }
}

boolean validatePurchaseOrderFields(CustomerPurchaseOrder purchaseOrder) {
    if (purchaseOrder.customerAccountId == null || purchaseOrder.customerAccountId.isEmpty())
        return false;
    if (purchaseOrder.shippingAddressId == null || purchaseOrder.shippingAddressId.isEmpty())
        return false;
    if (purchaseOrder.lineItems == null || purchaseOrder.lineItems.isEmpty())
        return false;
    return true;
}

float calculateOrderTotalBeforeDiscount(CustomerPurchaseOrder purchaseOrder) {
    float accumulatedTotal = 0.0f;
    for (OrderLineItem lineItem : purchaseOrder.lineItems) {
        float lineItemSubtotal = lineItem.unitPriceAtOrderTime * lineItem.requestedQuantity;
        accumulatedTotal += lineItemSubtotal;
    }
    return accumulatedTotal;
}

void applyPromotionalDiscountToOrder(CustomerPurchaseOrder purchaseOrder,
                                     float promotionalDiscountPercentage) {
    float discountAmountToDeduct = purchaseOrder.totalAmountBeforeDiscount
                                 * (promotionalDiscountPercentage / 100.0f);
    purchaseOrder.totalAmountAfterDiscount =
        purchaseOrder.totalAmountBeforeDiscount - discountAmountToDeduct;
}

void confirmAndFinaliseCustomerOrder(CustomerPurchaseOrder purchaseOrder,
                                     String generatedOrderReferenceId) {
    purchaseOrder.orderReferenceId   = generatedOrderReferenceId;
    purchaseOrder.currentOrderStatus = "CONFIRMED";
}

void cancelCustomerOrderWithReason(CustomerPurchaseOrder purchaseOrder,
                                   String cancellationReasonCode,
                                   String cancellationReasonDescription) {
    purchaseOrder.currentOrderStatus = "CANCELLED";
}
