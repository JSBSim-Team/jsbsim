#include "FDMTypes.h"

const FSimpleWindState FSimpleWindState::Calm = FSimpleWindState();
const FSimpleWindState FSimpleWindState::StandardEastZephyr = FSimpleWindState(ETurbType::Standard, 0.0, 0.0, FVector::RightVector, 0.0);
const FSimpleWindState FSimpleWindState::StandardWestZephyr = FSimpleWindState(ETurbType::Standard, 0.0, 0.0, FVector::RightVector * -1.0, 0.0);
const FSimpleWindState FSimpleWindState::StandardNorthZephyr = FSimpleWindState(ETurbType::Standard, 0.0, 0.0, FVector::ForwardVector, 0.0);
const FSimpleWindState FSimpleWindState::StandardSouthZephyr = FSimpleWindState(ETurbType::Standard, 0.0, 0.0, FVector::ForwardVector * -1.0, 0.0);