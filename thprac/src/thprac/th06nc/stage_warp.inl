// Original TH06 stage portions; ECLWarp translates to the NC timeline.
    __declspec(noinline) void THStageWarp([[maybe_unused]] ECLHelper& ecl, int stage, int portion)
    {
        if (stage == 1) {
            switch (portion) {
            case 1:
                ECLWarp(68);
                break;
            case 2:
                ECLWarp(580);
                break;
            case 3:
                ECLWarp(1160);
                break;
            case 4:
                ECLWarp(1540);
                break;
            case 5:
                ECLWarp(2348);
                break;
            case 6:
                ECLWarp(4438);
                break;
            default:
                break;
            }
        } else if (stage == 2) {
            switch (portion) {
            case 1:
                ECLWarp(270);
                break;
            case 2:
                ECLWarp(924);
                break;
            case 3:
                ECLWarp(3528);
                break;
            case 4:
                ECLWarp(4563);
                break;
            default:
                break;
            }
        } else if (stage == 3) {
            switch (portion) {
            case 1:
                ECLWarp(340);
                break;
            case 2:
                ECLWarp(1050);
                break;
            case 3:
                ECLWarp(1670);
                break;
            case 4:
                ECLWarp(2762);
                break;
            case 5:
                ECLWarp(3807);
                break;
            case 6:
                ECLWarp(4118);
                break;
            case 7:
                ECLWarp(5274);
                break;
            default:
                break;
            }
        } else if (stage == 4) {
            switch (portion) {
            case 1:
                ECLWarp(380);
                break;
            case 2:
                ECLWarp(1454);
                break;
            case 3:
                ECLWarp(2328);
                break;
            case 4:
                ECLWarp(0x0d40);
                break;
            case 5:
                ECLWarp(4872);
                break;
            case 6:
                ECLWarp(5712);
                break;
            case 7:
                ECLWarp(7434);
                break;
            case 8:
                ECLWarp(8354);
                break;
            case 9:
                ECLWarp(9784);
                break;
            default:
                break;
            }
        } else if (stage == 5) {
            switch (portion) {
            case 1:
                ECLWarp(350);
                break;
            case 2:
                ECLWarp(1352);
                break;
            case 3:
                ECLWarp(2292);
                break;
            case 4:
                ECLWarp(3814);
                break;
            case 5:
                ECLWarp(6774);
                break;
            default:
                break;
            }
        } else if (stage == 6) {
            switch (portion) {
            case 1:
                ECLWarp(380);
                break;
            case 2:
                ECLWarp(1484);
                break;
            default:
                break;
            }
        } else if (stage == 7) {
            switch (portion) {
            case 1:
                ECLWarp(380);
                break;
            case 2:
                ECLWarp(1300);
                break;
            case 3:
                ECLWarp(2600);
                break;
            case 4:
                ECLWarp(3680);
                break;
            case 5:
                ECLWarp(4803);
                break;
            case 6:
                ECLWarp(5933);
                break;
            case 7:
                ECLWarp(7733);
                break;
            default:
                break;
            }
        }
    }
