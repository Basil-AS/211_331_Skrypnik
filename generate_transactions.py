
import csv
import hashlib
import random
import time

def generate_transactions(filename, num_transactions, create_invalid_file=False):

    routes = [
        "Москва—ВеликиеЛуки—Псков—Москва",
        "Москва—Киров—Ростов—ПереславльЗалесский—Москва", 
        "Москва—Киров—ВеликийУстюг—Москва",
        "Ижевск—Воткинск",
        "Слюдянка1—Байкал", 
        "Волгоград1—Саратов",
        "Красноярск—Дивногорск",
        "Москва—Тамань—Геленджик—Новороссийск",
        "Москва—Майкоп—Владикавказ—Грозный—Махачкала—Назрань",
        "Москва—Майкоп—Владикавказ—Грозный—Дербент—Назрань"
    ]
    
    previous_hash = ''
    
    with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.writer(csvfile)
        for i in range(num_transactions):
            card_number = ''.join([str(random.randint(0, 9)) for _ in range(16)])
            route = random.choice(routes)
            transaction_time = int(time.time()) - random.randint(0, 31536000)
            
            data_to_hash = f"{card_number},{route},{transaction_time},{previous_hash}"
            current_hash = hashlib.md5(data_to_hash.encode()).hexdigest()
            
            if create_invalid_file and i == num_transactions // 2:
                writer.writerow([card_number, route, transaction_time, "invalid" + current_hash[7:]])
            else:
                writer.writerow([card_number, route, transaction_time, current_hash])
            
            previous_hash = current_hash

if __name__ == '__main__':
    generate_transactions('transactions.csv', 6)
