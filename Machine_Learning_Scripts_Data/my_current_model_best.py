import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout
from tensorflow.keras.optimizers.legacy import Adam
from tensorflow.keras.callbacks import ReduceLROnPlateau

EPOCHS = args.epochs or 30
LEARNING_RATE = args.learning_rate or 0.002
ENSURE_DETERMINISM = args.ensure_determinism
BATCH_SIZE = args.batch_size or 128

if not ENSURE_DETERMINISM:
    train_dataset = train_dataset.shuffle(buffer_size=BATCH_SIZE*4)

train_dataset = train_dataset.batch(BATCH_SIZE, drop_remainder=False)
validation_dataset = validation_dataset.batch(BATCH_SIZE, drop_remainder=False)


model = Sequential()

model.add(Dense(40, activation='relu', input_shape=(input_length,)))
model.add(Dropout(0.25))

model.add(Dense(20, activation='relu'))
model.add(Dropout(0.15))

model.add(Dense(16, activation='relu'))
model.add(Dropout(0.2))
model.add(Dense(classes, name='y_pred', activation='softmax'))

class_weight = {
    0: 0.6,
    1: 2.0
}

opt = Adam(learning_rate=LEARNING_RATE, beta_1=0.9, beta_2=0.999)

# Add learning rate scheduler
lr_scheduler = ReduceLROnPlateau(
    monitor='val_loss',
    factor=0.5,
    patience=5,
    min_lr=0.0001,
    verbose=1
)

callbacks.append(BatchLoggerCallback(BATCH_SIZE, train_sample_count, epochs=EPOCHS, ensure_determinism=ENSURE_DETERMINISM))
callbacks.append(lr_scheduler)

model.compile(loss='categorical_crossentropy', optimizer=opt, metrics=['accuracy'])
model.fit(train_dataset, epochs=EPOCHS, validation_data=validation_dataset, verbose=2, 
          callbacks=callbacks, class_weight=class_weight)

disable_per_channel_quantization = False