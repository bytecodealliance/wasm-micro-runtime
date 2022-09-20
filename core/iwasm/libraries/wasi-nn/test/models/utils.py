import tensorflow as tf
import pathlib


def save_model(model, filename):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    tflite_models_dir = pathlib.Path("./")
    tflite_model_file = tflite_models_dir/filename
    tflite_model_file.write_bytes(tflite_model)
