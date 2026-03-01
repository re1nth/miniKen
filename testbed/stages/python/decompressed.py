



from dataclasses import dataclass, field
from typing import List


@dataclass
class OrderLineItem:
    product_sku_code: str
    product_display_name: str
    requested_quantity: int
    unit_price_at_order_time: float
    applicable_discount_rate: float = 0.0


@dataclass
class CustomerPurchaseOrder:
    order_reference_id: str
    customer_account_id: str
    shipping_address_id: str
    line_items: List[OrderLineItem] = field(default_factory=list)
    total_amount_before_discount: float = 0.0
    total_amount_after_discount: float = 0.0
    current_order_status: str = "PENDING"


def validate_purchase_order_fields(purchase_order: CustomerPurchaseOrder) -> bool:
    if not purchase_order.customer_account_id:
        return False
    if not purchase_order.shipping_address_id:
        return False
    if not purchase_order.line_items:
        return False
    return True


def calculate_order_total_before_discount(purchase_order: CustomerPurchaseOrder) -> float:
    accumulated_total = 0.0
    for line_item in purchase_order.line_items:
        line_item_subtotal = line_item.unit_price_at_order_time * line_item.requested_quantity
        accumulated_total += line_item_subtotal
    return accumulated_total


def apply_promotional_discount_to_order(
    purchase_order: CustomerPurchaseOrder,
    promotional_discount_percentage: float,
) -> None:
    discount_amount_to_deduct = (
        purchase_order.total_amount_before_discount
        * (promotional_discount_percentage / 100.0)
    )
    purchase_order.total_amount_after_discount = (
        purchase_order.total_amount_before_discount - discount_amount_to_deduct
    )


def confirm_and_finalise_customer_order(
    purchase_order: CustomerPurchaseOrder,
    generated_order_reference_id: str,
) -> None:
    purchase_order.order_reference_id = generated_order_reference_id
    purchase_order.current_order_status = "CONFIRMED"


def cancel_customer_order_with_reason(
    purchase_order: CustomerPurchaseOrder,
    cancellation_reason_code: str,
    cancellation_reason_description: str,
) -> None:
    purchase_order.current_order_status = "CANCELLED"
    _ = cancellation_reason_code
    _ = cancellation_reason_description
