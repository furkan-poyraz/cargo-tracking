#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Gonderim ve Musteri Yapilari
typedef struct Shipment {
    int shipmentID;
    char date[11];  // YYYY-MM-DD formatinda
    char status[20]; // "Teslim Edildi" veya "Teslim Edilmedi"
    int deliveryTime; // Gun cinsinden teslim suresi
    struct Shipment* next;
} Shipment;

typedef struct Customer {
    int customerID;
    char firstName[50];
    char lastName[50];
    Shipment* shipmentHistory; // Linked List (Gonderim Gecmisi)
    struct Customer* next;
} Customer;

int autoCustomerID = 1;
int autoShipmentID = 1;

// Priority Queue Veri Yapisi
typedef struct PriorityQueueNode {
    int shipmentID;
    int deliveryTime; // Gun cinsinden teslim suresi
    char status[20];  // "Isleme Alindi", "Teslimatta", "Teslim Edildi"
    struct PriorityQueueNode* next;
} PriorityQueueNode;

PriorityQueueNode* priorityQueue = NULL; // Kuyruk baslangici

// Sehir Dugumu Yapisi
typedef struct CityNode {
    int cityID;                // Sehir Kimligi
    char cityName[50];         // Sehir Adi
    int deliveryTime;          // Teslim suresi (gun cinsinden)
    struct CityNode* child;    // Ilk alt sehir (cocuk dugum)
    struct CityNode* sibling;  // Aynı seviyedeki sonraki sehir
} CityNode;

CityNode* root = NULL; // Agacin koku (kargo sirketinin merkezi)
int autoCityID = 1;

// Stack Veri Yapisi
typedef struct ShipmentStackNode {
    int shipmentID;
    char date[11];  // YYYY-MM-DD formatinda
    char status[20]; // "Teslim Edildi" veya "Teslim Edilmedi"
    int deliveryTime; // Gun cinsinden teslim suresi
    struct ShipmentStackNode* next;
} ShipmentStackNode;

ShipmentStackNode* shipmentStack = NULL; // Stack'in tepe noktasi

// Global degisken (musteri listesi baslangic noktasi)
Customer* customerList = NULL;

// Fonksiyon Prototipleri
void menu();
void initializeDefaultCities();
void printError(const char* message);
void* safeMalloc(size_t size);

void addCustomer(char* firstName, char* lastName);
Customer* findCustomer(int customerID);
void addShipment(int customerID, char* date, char* status, int deliveryTime);
void displayCustomerShipments(int customerID);

void addToPriorityQueue(int shipmentID, int deliveryTime, char* status);
void processPriorityQueue();
void displayPriorityQueue();
void freePriorityQueue();
void displayLastFiveShipments();

CityNode* createCityNode(int cityID, char* cityName);
void addCity(int parentCityID, int cityID, char* cityName, int deliveryTime);
CityNode* findCity(CityNode* node, int cityID);
int calculateTreeDepth(CityNode* node);
void printTree(CityNode* node, int level);
void freeCityTree(CityNode* node);
int countCities(CityNode* node);
void printCitiesAlphabetically(CityNode* node);
int calculateMinDeliveryTime(CityNode* node);

Shipment* searchDeliveredShipments(Shipment* shipmentHistory, const char* date);
void mergeSortShipments(Shipment** shipmentHistory);
Shipment* sortedMerge(Shipment* left, Shipment* right);
void splitList(Shipment* head, Shipment** frontRef, Shipment** backRef);

void pushShipment(int shipmentID, char* date, char* status, int deliveryTime);
void popShipment();
void displayShipmentStack();
void freeShipmentStack();

int isValidName(const char* name, int length);
int isValidDate(const char* date);
int isPositiveNumber(int number);
int countShipments(Shipment* shipment);

void freeCustomerList();
void cleanup();

int isValidName(const char* name, int length) {
    return strlen(name) < length && strlen(name) > 0; // 50 karakterlik sinir
}

int isValidDate(const char* date) {
    int year, month, day;

    // Tarih formatini cozumle
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3) {
        return 0; // Format hatali
    }

    // Ay ve gun kontrolu
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }

    // Gun sinirlarini daha kesin kontrol etmek
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
        return 0; // 30 gun iceren aylar
    }
    if (month == 2) {
        // Artik yil kontrolu
        int isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (day > (isLeap ? 29 : 28)) {
            return 0; // Subat ayi sinirlari
        }
    }

    return 1; // Gecerli tarih
}

int isPositiveNumber(int number) {
    return number > 0;
}

void* safeMalloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        printf("Bellek tahsisi basarisiz oldu.\n");
        exit(EXIT_FAILURE); // Programi guvenli sekilde sonlandir
    }
    return ptr;
}

void printError(const char* message) {
    printf("Hata: %s\n", message);
}

void freePriorityQueue() {
    while (priorityQueue != NULL) {
        PriorityQueueNode* temp = priorityQueue;
        priorityQueue = priorityQueue->next;
        free(temp);
    }
}

void freeCityTree(CityNode* node) {
    if (node == NULL) return;
    freeCityTree(node->child);
    freeCityTree(node->sibling);
    free(node);
}

void freeShipmentStack() {
    while (shipmentStack != NULL) {
        ShipmentStackNode* temp = shipmentStack;
        shipmentStack = shipmentStack->next;
        free(temp);
    }
}

void freeCustomerList() {
    while (customerList != NULL) {
        Customer* temp = customerList;
        customerList = customerList->next;

        Shipment* shipment = temp->shipmentHistory;
        while (shipment != NULL) {
            Shipment* tempShipment = shipment;
            shipment = shipment->next;
            free(tempShipment);
        }
        free(temp);
    }
}

void cleanup() {
    freePriorityQueue();
    freeCityTree(root);
    freeShipmentStack();
    freeCustomerList();
}

// Menu yapisi
void menu() {
    int choice;
    while (1) {
        printf("\n=== Kargo Yonetim Sistemi ===\n");
        printf("1. Yeni musteri ekle\n");
        printf("2. Kargo gonderimi ekle\n");
        printf("3. Gonderim gecmisini goruntule\n");
		printf("---\n");
		printf("4. Kargo ekle (Priority Queue)\n");
		printf("5. Oncelikli kargoyu isle\n");
		printf("6. Kuyruktaki kargolari listele\n");
		printf("7. Son 5 gonderiyi goruntule\n");
		printf("---\n");
		printf("8. Yeni sehir ekle\n");
		printf("9. Teslimat rotalarini goruntule\n");
		printf("10. Agac derinligini hesapla\n");
		printf("---\n");
		printf("11. Gonderim ekle (Stack)\n");
		printf("12. Stack'ten gonderim cikar\n");
		printf("13. Stack'teki gonderimleri listele\n");
		printf("---\n");
		printf("14. Teslim edilmis kargolari tarih ile ara\n");
		printf("15. Teslim edilmemis kargolari sirala\n");
		printf("---\n");
		printf("16. Teslimat rotasindaki toplam sehir sayisini hesapla\n");
		printf("17. En uzun teslimat rotasinin uzunlugunu hesapla\n");
		printf("18. Gonderim gecmisindeki toplam kargo sayisini hesapla\n");
		printf("19. Sehirleri alfabetik sirayla yazdir\n");
		printf("20. En kisa teslimat suresini hesapla\n");
		printf("---\n");
        printf("21. Cikis\n");
        printf("Seciminiz: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                char firstName[50], lastName[50];
                printf("Ad: ");
                scanf("%s", firstName);

                if (!isValidName(firstName, 50)) {
                    printError("Ad 50 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                printf("Soyad: ");
                scanf("%s", lastName);

                if (!isValidName(lastName, 50)) {
                    printError("Soyad 50 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                addCustomer(firstName, lastName);
                break;
            }

            case 2: {
                int customerID, deliveryTime;
                char date[11], status[20];

                printf("Musteri ID: ");
                scanf("%d", &customerID);

                if (!isPositiveNumber(customerID)) {
                    printError("Musteri numarasi pozitif olmalidir.");
                    break;
                }

                printf("Tarih (YYYY-MM-DD): ");
                scanf("%s", date);

                if (!isValidDate(date)) {
                    printError("Tarih formati hatali. YYYY-MM-DD formatinda giriniz.");
                    break;
                }

                printf("Durum (Teslim Edildi/Teslim Edilmedi): ");
                scanf("%s", status);

                if (!isValidName(status, 20)) {
                    printError("Durum bilgisi 20 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                printf("Teslim Suresi (gun): ");
                scanf("%d", &deliveryTime);

                if (!isPositiveNumber(deliveryTime)) {
                    printError("Teslim suresi pozitif olmalidir.");
                    break;
                }

                addShipment(customerID, date, status, deliveryTime);
                break;
            }

            case 3: {
                int customerID;
                printf("Musteri ID: ");
                scanf("%d", &customerID);

                if (!isPositiveNumber(customerID)) {
                    printError("Musteri numarasi pozitif olmalidir.");
                    break;
                }

                displayCustomerShipments(customerID);
                break;
            }


            case 4: {
                int shipmentID, deliveryTime;
                char status[20];
                printf("Kargo ID: ");
                scanf("%d", &shipmentID);

                if (!isPositiveNumber(shipmentID)) {
                    printError("Kargo ID pozitif olmalidir.");
                    break;
                }

                printf("Teslim Suresi (gun): ");
                scanf("%d", &deliveryTime);

                if (!isPositiveNumber(deliveryTime)) {
                    printError("Teslim suresi pozitif olmalidir.");
                    break;
                }

                printf("Durum (Isleme Alindi/Teslimatta/Teslim Edildi): ");
                scanf("%s", status);

                if (!isValidName(status, 20)) {
                    printError("Durum bilgisi 20 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                addToPriorityQueue(shipmentID, deliveryTime, status);
                break;
            }

			case 5:
				processPriorityQueue();
				break;
			case 6:
				displayPriorityQueue();
				break;
			case 7:
				displayLastFiveShipments();
				break;

            case 8: {
                int parentCityID, cityID, deliveryTime;
                char cityName[50];
                printf("Ebeveyn Sehir ID: ");
                scanf("%d", &parentCityID);

                if (!isPositiveNumber(parentCityID)) {
                    printError("Ebeveyn sehir ID pozitif olmalidir.");
                    break;
                }

                printf("Yeni Sehir Adi: ");
                scanf("%s", cityName);

                if (!isValidName(cityName, 50)) {
                    printError("Sehir adi 50 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                printf("Teslimat Suresi: ");
                scanf("%d", &deliveryTime);

                if (!isPositiveNumber(deliveryTime)) {
                    printError("Teslimat suresi pozitif olmalidir.");
                    break;
                }

                addCity(parentCityID, 0, cityName, deliveryTime);
                break;
            }

			case 9:
				printf("Teslimat agaci yapisi:\n");
				printTree(root, 0);
				break;
			case 10:
				printf("Agac derinligi: %d\n", calculateTreeDepth(root));
				break;
            case 11: {
                int shipmentID, deliveryTime;
                char date[11], status[20];
                printf("Gonderi ID: ");
                scanf("%d", &shipmentID);

                if (!isPositiveNumber(shipmentID)) {
                    printError("Gonderi ID pozitif olmalidir.");
                    break;
                }

                printf("Tarih (YYYY-MM-DD): ");
                scanf("%s", date);

                if (!isValidDate(date)) {
                    printError("Tarih formati hatali. YYYY-MM-DD formatinda giriniz.");
                    break;
                }

                printf("Durum (Teslim Edildi/Teslim Edilmedi): ");
                scanf("%s", status);

                if (!isValidName(status, 20)) {
                    printError("Durum bilgisi 20 karakterden kisa olmalidir ve bos olmamalidir.");
                    break;
                }

                printf("Teslim Suresi (gun): ");
                scanf("%d", &deliveryTime);

                if (!isPositiveNumber(deliveryTime)) {
                    printError("Teslim suresi pozitif olmalidir.");
                    break;
                }

                pushShipment(shipmentID, date, status, deliveryTime);
                break;
            }

			case 12:
				popShipment();
				break;
			case 13:
				displayShipmentStack();
				break;

			case 14: {
				int customerID;
				char searchDate[11];
				printf("Musteri ID: ");
				scanf("%d", &customerID);

				if (!isPositiveNumber(customerID)) {
					printError("Musteri numarasi pozitif olmalidir.");
					break;
				}

				Customer* customer = findCustomer(customerID);
				if (customer == NULL) {
					printError("Musteri bulunamadi.");
					break;
				}

				printf("Aramak istediginiz tarih (YYYY-MM-DD): ");
				scanf("%s", searchDate);

				if (!isValidDate(searchDate)) {
					printError("Gecersiz tarih formati. YYYY-MM-DD seklinde giriniz.");
					break;
				}

				Shipment* result = searchDeliveredShipments(customer->shipmentHistory, searchDate);
				if (result) {
					printf("Teslim edilmis kargo bulundu: ID: %d, Tarih: %s, Durum: %s\n",
					   result->shipmentID, result->date, result->status);
				} else {
					printf("Teslim edilmis kargo bulunamadi.\n");
				}
				break;
			}
			case 15: {
				int customerID;
				printf("Musteri ID: ");
				scanf("%d", &customerID);

				if (!isPositiveNumber(customerID)) {
					printError("Musteri numarasi pozitif olmalidir.");
					break;
				}

				Customer* customer = findCustomer(customerID);
				if (customer == NULL) {
					printError("Musteri bulunamadi.");
					break;
				}

				mergeSortShipments(&customer->shipmentHistory);

				printf("Teslim edilmeyen kargolar siralandi:\n");
				Shipment* current = customer->shipmentHistory;
				while (current) {
					if (strcmp(current->status, "Teslim Edilmedi") == 0) {
						printf("ID: %d, Tarih: %s, Durum: %s, Teslim Suresi: %d\n",
						   current->shipmentID, current->date, current->status, current->deliveryTime);
					}
					current = current->next;
				}
				break;
			}

			case 16: {
				printf("Toplam sehir sayisi: %d\n", countCities(root));
				break;
			}
			case 17: {
				printf("En uzun teslimat rotasinin uzunlugu: %d\n", calculateTreeDepth(root));
				break;
			}
			case 18: {
				int customerID;
				printf("Musteri ID: ");
				scanf("%d", &customerID);

				if (!isPositiveNumber(customerID)) {
                    printError("Musteri numarasi pozitif olmalidir.");
                    break;
                }

				Customer* customer = findCustomer(customerID);
				if (customer == NULL) {
					printf("Musteri bulunamadi.\n");
				} else {
					printf("Gonderim gecmisindeki toplam kargo sayisi: %d\n", countShipments(customer->shipmentHistory));
				}
				break;
			}
			case 19:
				printf("Sehirler (alfabetik sirayla):\n");
				printCitiesAlphabetically(root);
				break;

			case 20: {
				int minDeliveryTime = calculateMinDeliveryTime(root);
				if (minDeliveryTime == INT_MAX) {
					printf("Teslimat agi bos.\n");
				} else {
					printf("En kisa teslimat suresi: %d gun\n", minDeliveryTime);
				}
				break;
			}
			case 21:
				printf("Sistemden cikis yapiliyor...\n");
				cleanup();
				exit(0);
            default:
                printf("Gecersiz secim. Lutfen tekrar deneyin.\n");
        }
    }
}


// Musteri ve gonderim islemleri
void addCustomer(char* firstName, char* lastName) {
    // Musteri ID'sini otomatik olarak belirle
    int customerID = autoCustomerID++;

    Customer* newCustomer = (Customer*)safeMalloc(sizeof(Customer));
    newCustomer->customerID = customerID;
    strcpy(newCustomer->firstName, firstName);
    strcpy(newCustomer->lastName, lastName);
    newCustomer->shipmentHistory = NULL;
    newCustomer->next = customerList;
    customerList = newCustomer;

    printf("Musteri %d (%s %s) basariyla eklendi.\n", customerID, firstName, lastName);
}

Customer* findCustomer(int customerID) {
    Customer* current = customerList;
    while (current != NULL) {
        if (current->customerID == customerID) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


void addShipment(int customerID, char* date, char* status, int deliveryTime) {
	int shipmentID = autoShipmentID++;

    Customer* customer = findCustomer(customerID);
    if (customer == NULL) {
        printError("Musteri bulunamadi.");
        return;
    }

    if (!isValidDate(date)) {
        printError("Tarih formati hatali. YYYY-MM-DD formatinda giriniz.");
        return;
    }

    Shipment* newShipment = (Shipment*)safeMalloc(sizeof(Shipment));
    newShipment->shipmentID = shipmentID;
    strcpy(newShipment->date, date);
    strcpy(newShipment->status, status);
    newShipment->deliveryTime = deliveryTime;
    newShipment->next = NULL;

    // Tarihe gore sirali ekleme
    Shipment** current = &customer->shipmentHistory;
    while (*current != NULL && strcmp((*current)->date, date) < 0) {
        current = &(*current)->next;
    }
    newShipment->next = *current;
    *current = newShipment;

    printf("Gonderi %d musteri %d icin basariyla eklendi.\n", shipmentID, customerID);
}


void displayCustomerShipments(int customerID) {
    Customer* customer = findCustomer(customerID);
    if (customer == NULL) {
        printf("Musteri ID %d bulunamadi.\n", customerID);
        return;
    }

    printf("Musteri: %s %s (ID: %d)\n", customer->firstName, customer->lastName, customer->customerID);
    printf("Gonderim Gecmisi:\n");

    Shipment* current = customer->shipmentHistory;
    if (current == NULL) {
        printf("  Gonderim gecmisi yok.\n");
        return;
    }

    while (current != NULL) {
        printf("  Gonderi ID: %d, Tarih: %s, Durum: %s, Teslim Suresi: %d gun\n",
               current->shipmentID, current->date, current->status, current->deliveryTime);
        current = current->next;
    }
}

// Kargo Ekleme (Priority Queue'ya ekleme)
void addToPriorityQueue(int shipmentID, int deliveryTime, char* status) {
    PriorityQueueNode* newNode = (PriorityQueueNode*)malloc(sizeof(PriorityQueueNode));
    newNode->shipmentID = shipmentID;
    newNode->deliveryTime = deliveryTime;
    strcpy(newNode->status, status);
    newNode->next = NULL;

    if (priorityQueue == NULL || deliveryTime < priorityQueue->deliveryTime) {
        // Yeni dugum basa ekleniyor
        newNode->next = priorityQueue;
        priorityQueue = newNode;
    } else {
        // Dogru konumu bul ve yerlestir
        PriorityQueueNode* current = priorityQueue;
        while (current->next != NULL && current->next->deliveryTime <= deliveryTime) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    printf("Kargo ID %d, teslim suresi %d gun ile kuyruga eklendi.\n", shipmentID, deliveryTime);
}

// Oncelikli Kargoyu Isleme Alma
void processPriorityQueue() {
    if (priorityQueue == NULL) {
        printf("Kuyrukta islenecek kargo yok.\n");
        return;
    }

    PriorityQueueNode* temp = priorityQueue;
    printf("Kargo ID %d isleniyor. Teslim suresi: %d gun, Durum: %s\n",
           temp->shipmentID, temp->deliveryTime, temp->status);

    priorityQueue = priorityQueue->next; // Kuyrugu ilerlet
    free(temp); // Bellegi serbest birak
}

// Kuyruktaki Tum Kargolari Listeleme
void displayPriorityQueue() {
    if (priorityQueue == NULL) {
        printf("Kuyrukta kargo yok.\n");
        return;
    }


    printf("Kuyruktaki Kargolar:\n");
    PriorityQueueNode* current = priorityQueue;
    while (current != NULL) {
        printf("  Kargo ID: %d, Teslim Suresi: %d gun, Durum: %s\n",
               current->shipmentID, current->deliveryTime, current->status);
        current = current->next;
    }
}


// Gonderileri tarihe gore siralar ve en son 5 kargoyu listeler
void displayLastFiveShipments() {
    Shipment* allShipments = NULL; // Tum kargolarin birlesik listesi
    Shipment* currentShipment;
    Customer* currentCustomer = customerList;

    // Tum musterilerin gonderim gecmislerini birlestir
    while (currentCustomer != NULL) {
        currentShipment = currentCustomer->shipmentHistory;
        while (currentShipment != NULL) {
            Shipment* newShipment = (Shipment*)safeMalloc(sizeof(Shipment));
            *newShipment = *currentShipment; // Verileri kopyala
            newShipment->next = allShipments;
            allShipments = newShipment;

            currentShipment = currentShipment->next;
        }
        currentCustomer = currentCustomer->next;
    }

    // Gonderimleri tarihe gore siralama
    mergeSortShipments(&allShipments);

    // Son 5 kargoyu yazdir
    printf("Son 5 gonderi:\n");
    currentShipment = allShipments;
    int count = 0;

    while (currentShipment != NULL && count < 5) {
        printf("Gonderi ID: %d, Tarih: %s, Durum: %s, Teslim Suresi: %d gun\n",
               currentShipment->shipmentID, currentShipment->date,
               currentShipment->status, currentShipment->deliveryTime);

        currentShipment = currentShipment->next;
        count++;
    }

    // Gecici birlesik listeyi serbest birak
    while (allShipments != NULL) {
        Shipment* temp = allShipments;
        allShipments = allShipments->next;
        free(temp);
    }
}


// Sehir Ekleme
CityNode* createCityNode(int cityID, char* cityName) {
	if (cityID > autoCityID) {
		autoCityID = cityID;
	}

    CityNode* newNode = (CityNode*)malloc(sizeof(CityNode));
    newNode->cityID = cityID;
    strcpy(newNode->cityName, cityName);
    newNode->child = NULL;
    newNode->sibling = NULL;
    return newNode;
}

void addCity(int parentCityID, int cityID, char* cityName, int deliveryTime) {
	if (cityID == 0) {
		cityID = autoCityID++;
	}
	if (cityID > autoCityID) {
		autoCityID = cityID;
	}

    if (root == NULL) {
		// Kok dugum olusturuluyor
        root = createCityNode(cityID, cityName);
        root->deliveryTime = deliveryTime;
        printf("Kargo merkezi olarak %s eklendi.\n", cityName);
        return;
    }

	// Ebeveyn dugumu bul
    CityNode* parent = findCity(root, parentCityID);
    if (parent == NULL) {
        printf("Ebeveyn sehir ID %d bulunamadi.\n", parentCityID);
        return;
    }

	// Yeni dugumu ekle
    CityNode* newNode = createCityNode(cityID, cityName);
    newNode->deliveryTime = deliveryTime;

    if (parent->child == NULL) {
        parent->child = newNode;
    } else {
        CityNode* sibling = parent->child;
        while (sibling->sibling != NULL) {
            sibling = sibling->sibling;
        }
        sibling->sibling = newNode;
    }

    printf("Sehir %s (ID: %d), %s ebeveyn sehrine bagli olarak eklendi (Teslim Suresi: %d gun).\n", cityName, cityID, parent->cityName, deliveryTime);
}

// Sehir Bulma
CityNode* findCity(CityNode* node, int cityID) {
    if (node == NULL) {
        return NULL;
    }

    if (node->cityID == cityID) {
        return node;
    }

    // Cocuklarda ara
    CityNode* found = findCity(node->child, cityID);
    if (found != NULL) {
        return found;
    }

    // Kardeslerde ara
    return findCity(node->sibling, cityID);
}


// Agac Yapisini Konsola Cizdirme
void printTree(CityNode* node, int level) {
    if (node == NULL) {
        return;
    }

    // Dugumu yazdir
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    printf("-> %s (ID: %d)\n", node->cityName, node->cityID);

    // Cocuklari yazdir
    printTree(node->child, level + 1);

    // Kardesleri yazdir
    printTree(node->sibling, level);
}

// Gonderim Ekleme (Push)
void pushShipment(int shipmentID, char* date, char* status, int deliveryTime) {
    ShipmentStackNode* newNode = (ShipmentStackNode*)malloc(sizeof(ShipmentStackNode));
    newNode->shipmentID = shipmentID;
    strcpy(newNode->date, date);
    strcpy(newNode->status, status);
    newNode->deliveryTime = deliveryTime;

    newNode->next = shipmentStack; // Yeni dugumu tepeye ekle
    shipmentStack = newNode;

    printf("Gonderi ID %d stack'e eklendi.\n", shipmentID);
}

//Gonderim Cikartma (Pop)
void popShipment() {
    if (shipmentStack == NULL) {
        printf("Stack bos, cikartilacak gonderim yok.\n");
        return;
    }

    ShipmentStackNode* temp = shipmentStack;
    printf("Gonderi ID %d cikariliyor. Tarih: %s, Durum: %s, Teslim Suresi: %d gun\n",
           temp->shipmentID, temp->date, temp->status, temp->deliveryTime);

    shipmentStack = shipmentStack->next; // Tepeyi bir sonraki dugume tasi
    free(temp); // Bellegi serbest birak
}

// Stack'teki Gonderimleri Listeleme
void displayShipmentStack() {
    if (shipmentStack == NULL) {
        printf("Stack bos, goruntulenecek gonderim yok.\n");
        return;
    }

    printf("Stack'teki Gonderimler:\n");
    ShipmentStackNode* current = shipmentStack;
    while (current != NULL) {
        printf("  Gonderi ID: %d, Tarih: %s, Durum: %s, Teslim Suresi: %d gun\n",
               current->shipmentID, current->date, current->status, current->deliveryTime);
        current = current->next;
    }
}

// Binary Search (Teslim Edilmis Kargolar icin)
Shipment* searchDeliveredShipments(Shipment* shipmentList, const char* targetDate) {
    Shipment* start = shipmentList;
    Shipment* end = NULL;

    while (start != end) {
        Shipment* mid = start;
        Shipment* temp = start;
        int count = 0;

        // Orta elemani bulmak icin
        while (temp != end) {
            temp = temp->next;
            if (count++ % 2 == 1) mid = mid->next;
        }

        // Tarihi kontrol et
        if (strcmp(mid->date, targetDate) == 0 && strcmp(mid->status, "Teslim Edildi") == 0) {
            return mid; // Aranan teslim edilmis kargo bulundu
        }

        // Tarih kucukse arama alanini daralt
        if (strcmp(mid->date, targetDate) < 0) {
            start = mid->next;
        } else {
            end = mid;
        }
    }

    return NULL; // Aranan teslim edilmis kargo bulunamadi
}


// Siralama Algoritmasi (Teslim Edilmemis Kargolar icin)
void mergeSortShipments(Shipment** headRef) {
    Shipment* head = *headRef;
    Shipment* a;
    Shipment* b;

    if (!head || !head->next) return;

    // Listeyi ortadan ikiye bol
    splitList(head, &a, &b);

    // Bolumleri sirala
    mergeSortShipments(&a);
    mergeSortShipments(&b);

    // Bolumleri birlestir
    *headRef = sortedMerge(a, b);
}

Shipment* sortedMerge(Shipment* a, Shipment* b) {
    Shipment* result = NULL;

    if (!a) return b;
    if (!b) return a;

    // Karsilastirma: teslim suresi kucuk olan onde
    if (strcmp(a->status, "Teslim Edilmedi") == 0 &&
        strcmp(b->status, "Teslim Edilmedi") == 0 &&
        a->deliveryTime <= b->deliveryTime) {
        result = a;
        result->next = sortedMerge(a->next, b);
    } else {
        result = b;
        result->next = sortedMerge(a, b->next);
    }

    return result;
}

void splitList(Shipment* source, Shipment** frontRef, Shipment** backRef) {
    Shipment* fast;
    Shipment* slow;
    slow = source;
    fast = source->next;

    // Hizli ve yavas pointer ile listeyi ikiye bol
    while (fast) {
        fast = fast->next;
        if (fast) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
}


// Teslimat Rotasinda Sehir Sayisini Bulma
int countCities(CityNode* node) {
    if (node == NULL) {
        return 0; // Agacin sonuna ulasildi
    }
    return 1 + countCities(node->child) + countCities(node->sibling);
}

// En Uzun Teslimat Rotasini Hesaplama
int calculateTreeDepth(CityNode* node) {
    if (node == NULL) {
        return 0;
    }

    int childDepth = calculateTreeDepth(node->child);
    int siblingDepth = calculateTreeDepth(node->sibling);

    return (childDepth + 1 > siblingDepth) ? childDepth + 1 : siblingDepth;
}

// Gonderim Gecmisindeki Kargo Sayisini Bulma
int countShipments(Shipment* shipment) {
    if (shipment == NULL) {
        return 0;
    }
    return 1 + countShipments(shipment->next);
}

// Sehir Isimlerini Alfabetik Siralama
void printCitiesAlphabetically(CityNode* node) {
    if (node == NULL) {
        return;
    }

    // Cocuklari sirala
    printCitiesAlphabetically(node->child);

    // Mevcut dugumu yazdir
    printf("Sehir: %s (ID: %d)\n", node->cityName, node->cityID);

    // Kardesleri sirala
    printCitiesAlphabetically(node->sibling);
}

// En Kisa Teslimat Suresi
int calculateMinDeliveryTime(CityNode* node) {
    if (node == NULL) {
        return INT_MAX; // Sonsuz olarak varsayilan en buyuk deger
    }

    int currentDeliveryTime = node->deliveryTime;
    int childMinTime = calculateMinDeliveryTime(node->child);
    int siblingMinTime = calculateMinDeliveryTime(node->sibling);

    return currentDeliveryTime < childMinTime ?
           (currentDeliveryTime < siblingMinTime ? currentDeliveryTime : siblingMinTime) :
           (childMinTime < siblingMinTime ? childMinTime : siblingMinTime);
}

// Varsayilan Sehirler
void initializeDefaultCities() {
    // Istanbul'u kok olarak ekle
    root = createCityNode(1, "Istanbul");
    root->deliveryTime = 1;
	printf("Kargo merkezi olarak Istanbul (ID: 1) eklendi.\n");

    // Ilk seviye sehirler
    addCity(1, 2, "Tekirdag", 2); // Istanbul'un altinda
    addCity(1, 3, "Edirne", 3);   // Istanbul'un altinda
    addCity(1, 4, "Kirklareli", 4); // Istanbul'un altinda

    // Istanbul'dan daha uzak olan sehirler
    addCity(1, 5, "Bursa", 5); // Istanbul'un altinda
    addCity(5, 6, "Canakkale", 6); // Bursa'nin altinda
    addCity(5, 7, "Balikesir", 4);     // Bursa'nin altinda
    addCity(5, 11, "Bilecik", 5);   // Bursa'nin altinda

    // Sakarya ve cevresi
    addCity(1, 8, "Sakarya", 3);    // Istanbul'un altinda
    addCity(8, 9, "Kocaeli", 2);    // Sakarya'nin altinda
    addCity(9, 10, "Yalova", 3);    // Kocaeli'nin altinda

    printf("Varsayilan sehirler ve iliskiler basariyla eklendi.\n");
}

// Ana fonksiyon
int main() {
	initializeDefaultCities();
    menu();
    return 0;
}
