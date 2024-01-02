#include <stdio.h>

// Define an enumeration type called 'Weekdays'
enum Weekdays {
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
};

int main() {
    // Declare a variable of type 'Weekdays'
    enum Weekdays today;

    // Assign a value from the enumeration to the variable
    today = TUESDAY;

    // Print the value of 'today'
    switch (today) {
        case MONDAY:
            printf("Today is Monday.\n");
            break;
        case TUESDAY:
            printf("Today is Tuesday.\n");
            break;
        case WEDNESDAY:
            printf("Today is Wednesday.\n");
            break;
        case THURSDAY:
            printf("Today is Thursday.\n");
            break;
        case FRIDAY:
            printf("Today is Friday.\n");
            break;
        case SATURDAY:
            printf("Today is Saturday.\n");
            break;
        case SUNDAY:
            printf("Today is Sunday.\n");
            break;
        default:
            printf("Invalid day.\n");
            break;
    }

    return 0;
}