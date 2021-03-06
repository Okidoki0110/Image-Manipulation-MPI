
Programul citește imaginea de intrare (tot fișierul ) complet în memorie.
   
    std::ifstream input_image;
    input_image.open(argv[1], std::ios::binary);
    if (!input_image.is_open()) { /* eroare */ }

    input_image.seekg(0, input_image.end);
    size_t input_image_size = input_image.tellg();
    input_image.seekg(0, input_image.beg);

    char * image_file_buffer = new(std::nothrow) char[input_image_size];
    if (image_file_buffer == NULL) {/* eroare */}

    input_image.read(image_file_buffer, input_image_size);
    input_image.close();


Apoi se parsează structura fișierului pnm/pgm.
Observație: fișierele .pnm/.pgm pot conține comentarii în structura lor, imediat după primul rând.
Acest lucru nu este specificat in enuțul temei.

Se creează o imagine bordată cu două rânduri (sus și jos) cu 0.

    int bytes_per_row = image_width * (image_bw ? 1 : 3);

    int padded_image_size = bytes_per_row * (image_height + 2);
    char * padded_image_buffer = new(std::nothrow) char[padded_image_size];
    if (padded_image_buffer == NULL) {
      std::cerr << "Unable to allocate memory for image!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    memset(padded_image_buffer, 0, bytes_per_row);
    memcpy(padded_image_buffer + bytes_per_row, image_buffer, bytes_per_row * image_height);
    memset(padded_image_buffer + (bytes_per_row * image_height) + bytes_per_row, 0, bytes_per_row); 

padded_image_buffer este un pointer care contine noua imagine bordată, iar image_buffer este pointerul către imaginea originală.
După bordare se poate elibera memorie ocupată de imaginea inițială. ```( delete[] image_file_buffer; )```


Distribuirea imaginii se face pe procesul cu rank 0.
Se calculează numărul de rânduri care revine fiecărui proces.

    int rows_per_thread = image_height / NThreads;
    int part_image_size = rows_per_thread * bytes_per_row;

Procesul cu rank 1 primește partea superioară de imagine.
Observație: împărțirea imaginii se face de la procesul cu rank 1.
Ultima parte de imagine va aparține procesului cu rank 0.

Dacă se rulează programul cu un singur proces, atunci toată imaginea va aparține procesului cu rank 0.


Când se împarte imaginea, fiecare proces primește două rânduri de pixeli în plus:

- rândul superior aparține procesului anterior (thread_id - 1), dar avem nevoie de el pentru a calcula corect filtrele.
- rândul inferior aparține procesului următor (sau procesului cu rank0 dacă procesul actual are rank-ul NThreads-1; (thread_id + 1)%NThreads )

procesul cu rank 0 nu va avea rând inferior (doar padding-ul) indiferent de numărul de procese cu care rulează programul

procesul cu rank 1 nu va avea rând superior comun cu alt proces (dacă numărul de procese este mai mare ca 1; altfel nu exista procesul 1)

După ce se aplică filtrul pe imaginea care aparțne fiecărui proces în parte, procesele comunică mai departe acele rânduri (inferior + superior)
care se intersectează cu alt proces.

      MPI_Recv(input_image_part + working_image_size - bytes_per_row, bytes_per_row, MPI_BYTE, (thread_id + 1) % NThreads, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(input_image_part + working_image_size - 2 * bytes_per_row, bytes_per_row, MPI_BYTE, (thread_id + 1) % NThreads, 0, MPI_COMM_WORLD);

      if (thread_id != 1) {
        MPI_Send(input_image_part + bytes_per_row, bytes_per_row, MPI_BYTE, thread_id - 1, 0, MPI_COMM_WORLD);
        MPI_Recv((void * ) input_image_part, bytes_per_row, MPI_BYTE, thread_id - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }


input_image_part este partea de imagine specifica fiecărui proces.
primul rând aflat la acest pointer este rândul comun (sau padding-ul pentru procesul cu rank 1)
imaginea propriuzisă începe de la input_image_part + bytes_per_row;

working_image_size reprezintă dimensiunea totală a imaginii (cu tot cu capetele comune).

După aplicarea ultimului filtru nu se mai face comunicarea rândurilor comune pentru că nu mai este nevoie.


După ce a terminat de aplicat fiecare filtru, procesele trimit partea lor de imagine înapoi la procesul cu rank 0.
```  MPI_Send(input_image_part + bytes_per_row, working_image_size - 2 * bytes_per_row, MPI_BYTE, 0, 0, MPI_COMM_WORLD);```


Acesta reconstituie imaginea finală și o salvează în fișierul argv[2].
```
   if (NThreads > 1) {
      for (int i = 1; i < NThreads; i++) {
        MPI_Recv(image_buffer + bytes_per_row + ((i - 1) * rows_per_thread * bytes_per_row), rows_per_thread * bytes_per_row, MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }


    char resolution_ascii[64];
    std::ofstream output_image_file;
    output_image_file.open(argv[2], std::ios::binary);
    if (!output_image_file.is_open()) {/* eroare */}

    if (image_bw) {
      output_image_file.write("P5\n", 3);
    } else {
      output_image_file.write("P6\n", 3);
    }

    sprintf(resolution_ascii, "%d %d\n", image_width, image_height);

    output_image_file.write(resolution_ascii, strlen(resolution_ascii));
    output_image_file.write("255\n", 4);

    // se incepe dupa padding
    output_image_file.write(image_buffer + bytes_per_row, image_height * bytes_per_row); 
    output_image_file.close();


```

Programul a fost testat și corespunde cu cerințele.
Imaginea finală este corectă indiferent de numărul de thread-uri și programul poate scala.

Pot exista mici diferențe între fișierele de referință și output-ul programului din cauza erorilor de rotunjire.
Programul folosește tipul de date double pentru a stoca numerele cu virgulă mobilă.

De asemenea se face "bound checking" la transformarea din double în uint8_t și anume:
Dacă rezultatul va avea valoare negativă atunci funcția va întoarce 0, 
iar dacă rezultatul va avea valoare mai mare de 255 atunci funcția va întroarce 255.

Rezultatele programului sunt deterministice și repetabile de la o rulare la alta.
Nu sunt probleme de comunicare, rânduri diferite etc.



Pentru a măsura scalabilitatea s-au folosit imagni de următoarele dimensiuni:

  - landscape.pnm (3840x2160) ; cea mai mare imagine color care face parte din exemple
  - rorschach.pgm (3853x2000) ; cea mai mare imagine alb-negru din exemple	


Fiecare timp de execuție din acest tabel este egal cu media aritmetică a 5 măsurători. 
Pentru a calcula scalabilitatea se testează programul cu filtrele "blur smooth sharpen emboss mean blur smooth sharpen emboss mean".


Rezultate scalabilitate :

  | NR. PROCESE | landscape.pnm | rorschach.pgm | 
  | ----------- | ------------- | ------------- |
  |      1      |     2,454s    |     0,961s    |
  |      2      |     1,485s	  |     0,701s    |	
  |      3      |     1,134s	  |     0,622s    |
  |      4      |     1,030s	  |     0,562s    |
  |      5      |     1,282s	  |     0,634s    |
  |      6      |     1,194s    |     0,585s    |
  |      7      |     1,166s    |     0,563s    |
  |      8      |     1,029s    |     0,548s    |

Pentru a calcula acest tabel s-a folosit un laptop Lenovo IdeaPad L340 cu procesor Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz cu 4 nuclee și 8 thread-uri, 8GB RAM DDR4 și SSD Silicon Power A55 1TB SATA3.

